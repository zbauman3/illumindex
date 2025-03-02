#pragma once

#include "esp_http_client.h"

// 28KiB = (64 * 64) values of 65,535 plus some
#define REQUEST_MAX_OUTPUT_BUFFER (1024 * 28)

typedef struct {
  size_t length;
  char *data;
  int statusCode;
} ResponseData;

typedef struct {
  char *url;
  esp_http_client_method_t method;
  ResponseData *response;
} RequestContext;

typedef RequestContext *RequestContextHandle;

esp_err_t requestInit(RequestContextHandle *ctxHandle);
esp_err_t requestEnd(RequestContextHandle ctx);
esp_err_t requestPerform(RequestContextHandle ctx);