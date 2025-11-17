#pragma once

#include "esp_http_client.h"

// MD5 as base64, plus null terminator
#define ETAG_LENGTH 33

typedef struct {
  size_t length;
  char *data;
  int status_code;
  char *etag;
} fetch_response_data_t;

typedef struct {
  char *url;
  esp_http_client_method_t method;
  fetch_response_data_t *response;
  char *etag;
} fetch_ctx_t;

typedef fetch_ctx_t *fetch_ctx_handle_t;

esp_err_t fetch_init(fetch_ctx_handle_t *ctx_handle);
esp_err_t fetch_end(fetch_ctx_handle_t ctx);
esp_err_t fetch_perform(fetch_ctx_handle_t ctx);
esp_err_t fetch_etag_init(char **etag);
void fetch_etag_copy(char *to, char *from);
void fetch_etag_end(char **etag);