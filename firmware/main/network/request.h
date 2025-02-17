#pragma once

#include "esp_http_client.h"

#define REQUEST_MAX_OUTPUT_BUFFER 2048

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