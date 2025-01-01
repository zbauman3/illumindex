#pragma once

#define REQUEST_MAX_OUTPUT_BUFFER 2048

#define requestCtxCreate(_url)                                                 \
  {                                                                            \
      .url = _url,                                                             \
      .data = (char *)malloc(REQUEST_MAX_OUTPUT_BUFFER),                       \
      .length = REQUEST_MAX_OUTPUT_BUFFER,                                     \
  }

#define requestCtxCleanup(ctx) free(ctx.data)

typedef struct {
  char *url;
  size_t length;
  char *data;
} request_ctx_t;

esp_err_t getRequest(request_ctx_t *ctx);