#include "esp_crt_bundle.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_tls.h"
#include <string.h>
#include <sys/param.h>

#include "esp_http_client.h"

#include "network/request.h"

static const char *TAG = "NETWORK_REQUEST";

static esp_err_t eventHandler(esp_http_client_event_t *evt) {
  // Stores number of bytes read
  static int outputLen = 0;

  switch (evt->event_id) {
  case HTTP_EVENT_ERROR: {
    ESP_LOGW(TAG, "HTTP_EVENT_ERROR \"%s\"", (char *)evt->data);
    break;
  }
  case HTTP_EVENT_ON_DATA: {
    ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);

    request_ctx_t *ctx = evt->user_data;

    // Clean the buffer in case of a new request
    if (outputLen == 0) {
      memset(ctx->data, 0, ctx->length);
    }

    // The last byte in ctx->data is kept for the NULL character in case
    // of out-of-bound access.
    int copyLen = MIN(evt->data_len, MAX((ctx->length - outputLen), 0));
    if (copyLen) {
      memcpy(ctx->data + outputLen, evt->data, copyLen);
    }

    outputLen += copyLen;

    break;
  }
  case HTTP_EVENT_ON_FINISH: {
    ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
    outputLen = 0;
    break;
  }
  case HTTP_EVENT_DISCONNECTED: {
    int mbedtlsErr = 0;
    esp_err_t err = esp_tls_get_and_clear_last_error(
        (esp_tls_error_handle_t)evt->data, &mbedtlsErr, NULL);
    if (err != ESP_OK) {
      ESP_LOGW(TAG, "HTTP_EVENT_DISCONNECTED - err: 0x%x - mbedtls: 0x%x", err,
               mbedtlsErr);
    }
    outputLen = 0;
    break;
  }
  default: {
    ESP_LOGD(TAG, "EVENT - %d", evt->event_id);
    break;
  }
  }
  return ESP_OK;
}

esp_err_t getRequest(request_ctx_t *ctx) {
  esp_http_client_config_t config = {
      .url = ctx->url,
      .method = HTTP_METHOD_GET,
      .timeout_ms = 10000,
      .event_handler = eventHandler,
      .crt_bundle_attach = esp_crt_bundle_attach,
      .user_data = ctx,
  };
  esp_http_client_handle_t client = esp_http_client_init(&config);
  esp_err_t err = esp_http_client_perform(client);

  int statusCode = esp_http_client_get_status_code(client);
  esp_http_client_cleanup(client);

  if (err == ESP_OK) {
    ESP_LOGD(TAG, "(%d) \"%s\"", statusCode, ctx->url);
  } else {
    ESP_LOGE(TAG, "(%d) \"%s\"(%s) ", statusCode, ctx->url,
             esp_err_to_name(err));
  }

  return err;
}