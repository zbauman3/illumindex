#pragma once

#include "esp_http_client.h"

// 200 KB should be roughly enough?
#define REQUEST_MAX_OUTPUT_BUFFER (1024 * 200)
// MD5 as base64, plus null terminator
#define ETAG_LENGTH 33

typedef struct {
  size_t length;
  char *data;
  int statusCode;
  char *eTag;
} ResponseData;

typedef struct {
  char *url;
  esp_http_client_method_t method;
  ResponseData *response;
  char *eTag;
} RequestContext;

typedef RequestContext *RequestContextHandle;

esp_err_t requestInit(RequestContextHandle *ctxHandle);
esp_err_t requestEnd(RequestContextHandle ctx);
esp_err_t requestPerform(RequestContextHandle ctx);
void requestEtagInit(char **eTag);
void requestEtagCopy(char *to, char *from);