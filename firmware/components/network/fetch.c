#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_tls.h"
#include <string.h>
#include <sys/param.h>

#include "network/fetch.h"

static const char *TAG = "NETWORK_REQUEST";

void fetch_etag_init(char **etag) {
  fetch_etag_end(etag);
  *etag = (char *)malloc(sizeof(char) * ETAG_LENGTH);
}

void fetch_etag_copy(char *to, char *from) { memcpy(to, from, ETAG_LENGTH); }

void fetch_etag_end(char **etag) {
  free(*etag);
  *etag = NULL;
}

static esp_err_t event_handler(esp_http_client_event_t *evt) {
  switch (evt->event_id) {
  case HTTP_EVENT_ERROR: {
    ESP_LOGW(TAG, "HTTP_EVENT_ERROR");
    break;
  }
  case HTTP_EVENT_ON_DATA: {
    ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);

    fetch_ctx_handle_t ctx = evt->user_data;
    if (ctx->response->data == NULL) {
      ctx->response->data = (char *)malloc(evt->data_len);
    } else {
      ctx->response->data = (char *)realloc(
          ctx->response->data, ctx->response->length + evt->data_len);
    }

    // add the new data.
    memcpy(ctx->response->data + ctx->response->length, evt->data,
           evt->data_len);

    // update the length
    ctx->response->length += evt->data_len;

    break;
  }
  case HTTP_EVENT_ON_FINISH: {
    ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");

    fetch_ctx_handle_t ctx = evt->user_data;
    // if there is data, append the null terminator
    if (ctx->response->data != NULL) {
      ctx->response->length += 1;
      ctx->response->data =
          (char *)realloc(ctx->response->data, ctx->response->length);
      ctx->response->data[ctx->response->length - 1] = '\0';
    }
    break;
  }
  case HTTP_EVENT_ON_HEADER: {
    if (strcmp("etag", evt->header_key) == 0) {
      fetch_ctx_handle_t ctx = evt->user_data;
      size_t length = strlen(evt->header_value) + 1;

      if (length > ETAG_LENGTH) {
        ESP_LOGW(TAG, "ETAG length '%u' is larger than '%u'", length - 1,
                 ETAG_LENGTH - 1);
      } else {
        fetch_etag_init(&ctx->response->etag);
        fetch_etag_copy(ctx->response->etag, evt->header_value);
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
    break;
  }
  default: {
    ESP_LOGD(TAG, "EVENT - %d", evt->event_id);
    break;
  }
  }
  return ESP_OK;
}

esp_err_t fetch_init(fetch_ctx_handle_t *ctx_handle) {
  fetch_ctx_handle_t ctx = (fetch_ctx_handle_t)malloc(sizeof(fetch_ctx_t));
  ctx->response =
      (fetch_response_data_t *)malloc(sizeof(fetch_response_data_t));
  ctx->response->data = NULL;
  ctx->response->length = 0;
  ctx->response->etag = NULL;
  ctx->etag = NULL;

  *ctx_handle = ctx;
  return ESP_OK;
}

esp_err_t fetch_end(fetch_ctx_handle_t ctx) {
  fetch_etag_end(&ctx->response->etag);
  fetch_etag_end(&ctx->etag);
  free(ctx->response->data);
  free(ctx->response);
  free(ctx);
  return ESP_OK;
}

esp_err_t fetch_perform(fetch_ctx_handle_t ctx) {
  esp_http_client_config_t config = {
      .url = ctx->url,
      .method = ctx->method,
      .timeout_ms = 10000,
      .event_handler = event_handler,
      .crt_bundle_attach = esp_crt_bundle_attach,
      .user_data = ctx,
  };
  esp_http_client_handle_t client = esp_http_client_init(&config);

  if (ctx->etag != NULL) {
    esp_http_client_set_header(client, "If-None-Match", ctx->etag);
  }

  esp_http_client_set_header(client, "Authorization",
                             "Bearer: " CONFIG_ENDPOINT_TOKEN);

  esp_err_t err = esp_http_client_perform(client);

  ctx->response->status_code = esp_http_client_get_status_code(client);
  esp_http_client_cleanup(client);

  if (err == ESP_OK) {
    ESP_LOGD(TAG, "(%d) \"%s\"", ctx->response->status_code, ctx->url);
  } else {
    ESP_LOGE(TAG, "(%d) \"%s\"(%s) ", ctx->response->status_code, ctx->url,
             esp_err_to_name(err));
  }

  return err;
}