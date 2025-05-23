#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_tls.h"
#include <string.h>
#include <sys/param.h>

#include "network/request.h"

static const char *TAG = "NETWORK_REQUEST";

void requestEtagInit(char **eTag) {
  if (*eTag != NULL) {
    free(*eTag);
  }
  *eTag = (char *)malloc(sizeof(char) * ETAG_LENGTH);
}

void requestEtagCopy(char *to, char *from) { memcpy(to, from, ETAG_LENGTH); }

static esp_err_t eventHandler(esp_http_client_event_t *evt) {
  // Stores number of bytes read
  static int outputLen = 0;

  switch (evt->event_id) {
  case HTTP_EVENT_ERROR: {
    ESP_LOGW(TAG, "HTTP_EVENT_ERROR");
    break;
  }
  case HTTP_EVENT_ON_DATA: {
    ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);

    RequestContextHandle ctx = evt->user_data;

    // Clean the buffer in case of a new request
    if (outputLen == 0) {
      memset(ctx->response->data, 0, ctx->response->length);
    }

    // The last byte in ctx->data is kept for the NULL character in case
    // of out-of-bound access.
    int copyLen =
        MIN(evt->data_len, MAX((ctx->response->length - outputLen), 0));
    if (copyLen) {
      memcpy(ctx->response->data + outputLen, evt->data, copyLen);
    }

    outputLen += copyLen;

    break;
  }
  case HTTP_EVENT_ON_FINISH: {
    ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");

    // Now that we're done, update the length to represent the data's size
    RequestContextHandle ctx = evt->user_data;
    ctx->response->length = outputLen;

    outputLen = 0;
    break;
  }
  case HTTP_EVENT_ON_HEADER: {
    if (strcmp("etag", evt->header_key) == 0) {
      RequestContextHandle ctx = evt->user_data;
      size_t length = strlen(evt->header_value) + 1;

      if (length > ETAG_LENGTH) {
        ESP_LOGW(TAG, "ETAG length '%u' is larger than '%u'", length - 1,
                 ETAG_LENGTH - 1);
      } else {
        requestEtagInit(&ctx->response->eTag);
        requestEtagCopy(ctx->response->eTag, evt->header_value);
      }
    }
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

esp_err_t requestInit(RequestContextHandle *ctxHandle) {
  RequestContextHandle ctx =
      (RequestContextHandle)malloc(sizeof(RequestContext));
  ctx->response = (ResponseData *)malloc(sizeof(ResponseData));
  ctx->response->data = (char *)malloc(REQUEST_MAX_OUTPUT_BUFFER);
  ctx->response->length = REQUEST_MAX_OUTPUT_BUFFER;
  ctx->response->eTag = NULL;
  ctx->eTag = NULL;

  *ctxHandle = ctx;
  return ESP_OK;
}

esp_err_t requestEnd(RequestContextHandle ctx) {
  free(ctx->response->data);
  if (ctx->response->eTag != NULL) {
    free(ctx->response->eTag);
  }
  if (ctx->eTag != NULL) {
    free(ctx->eTag);
  }
  free(ctx->response);
  free(ctx);
  return ESP_OK;
}

esp_err_t requestPerform(RequestContextHandle ctx) {
  esp_http_client_config_t config = {
      .url = ctx->url,
      .method = ctx->method,
      .timeout_ms = 10000,
      .event_handler = eventHandler,
      .crt_bundle_attach = esp_crt_bundle_attach,
      .user_data = ctx,
  };
  esp_http_client_handle_t client = esp_http_client_init(&config);

  if (ctx->eTag != NULL) {
    esp_http_client_set_header(client, "If-None-Match", ctx->eTag);
  }

#ifdef CONFIG_ENDPOINT_TOKEN
  esp_http_client_set_header(client, "Authorization",
                             "Bearer: " CONFIG_ENDPOINT_TOKEN);
#endif

  esp_err_t err = esp_http_client_perform(client);

  ctx->response->statusCode = esp_http_client_get_status_code(client);
  esp_http_client_cleanup(client);

  if (err == ESP_OK) {
    ESP_LOGD(TAG, "(%d) \"%s\"", ctx->response->statusCode, ctx->url);
  } else {
    ESP_LOGE(TAG, "(%d) \"%s\"(%s) ", ctx->response->statusCode, ctx->url,
             esp_err_to_name(err));
  }

  return err;
}