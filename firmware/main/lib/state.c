

#include "cJSON.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"
#include <string.h>

#include "network/fetch.h"

#include "lib/state.h"

static const char *TAG = "STATE";

esp_err_t state_init(state_handle_t *state_handle) {
  // allocate the the state
  state_handle_t state = (state_handle_t)malloc(sizeof(state_t));

  state->is_dev_mode = false;
  state->commandEndpoint = CONFIG_ENDPOINT_URL;
  state->fetchInterval = CONFIG_ENDPOINT_FETCH_INTERVAL;
  state->remote_state_invalid = false;
  state->commands_invalid = false;
  state->loopSeconds = 65535;
  state->fetchFailureCount = 0;

  if (state->fetchInterval < 5) {
    ESP_LOGW(TAG, "Fetch interval was less than 5. Setting to 5.");
    state->fetchInterval = 5;
  }

  // pass back the state
  *state_handle = state;

  return ESP_OK;
}

void state_end(state_handle_t state) { free(state); }

esp_err_t parse_from_remote(state_handle_t state, char *data, size_t length) {
  esp_err_t ret = ESP_OK;
  const cJSON *values = NULL;
  cJSON *json = cJSON_ParseWithLength(data, length);

  ESP_GOTO_ON_FALSE(json != NULL, ESP_ERR_INVALID_RESPONSE,
                    parse_from_remote_cleanup, TAG,
                    "Invalid JSON response or content length");

  ESP_GOTO_ON_FALSE(cJSON_IsObject(json), ESP_ERR_INVALID_RESPONSE,
                    parse_from_remote_cleanup, TAG,
                    "JSON response is not an object");

  values = cJSON_GetObjectItemCaseSensitive(json, "isDevMode");
  ESP_GOTO_ON_FALSE(cJSON_IsBool(values), ESP_ERR_INVALID_RESPONSE,
                    parse_from_remote_cleanup, TAG,
                    "data.isDevMode is not a boolean");

  state->is_dev_mode = cJSON_IsTrue(values);

  values = cJSON_GetObjectItemCaseSensitive(json, "commandEndpoint");
  ESP_GOTO_ON_FALSE(cJSON_IsString(values) && values->valuestring != NULL,
                    ESP_ERR_INVALID_RESPONSE, parse_from_remote_cleanup, TAG,
                    "data.commandEndpoint is not a string");
  ESP_GOTO_ON_FALSE(strlen(values->valuestring) > 0, ESP_ERR_INVALID_RESPONSE,
                    parse_from_remote_cleanup, TAG,
                    "data.commandEndpoint is empty");
  // allocate the new endpoint
  state->commandEndpoint = (char *)malloc(strlen(values->valuestring) + 1);
  strcpy(state->commandEndpoint, values->valuestring);

  values = cJSON_GetObjectItemCaseSensitive(json, "fetchInterval");
  ESP_GOTO_ON_FALSE(cJSON_IsNumber(values), ESP_ERR_INVALID_RESPONSE,
                    parse_from_remote_cleanup, TAG,
                    "data.fetchInterval is not a number");
  ESP_GOTO_ON_FALSE(values->valueint > 0 && values->valueint <= 65535,
                    ESP_ERR_INVALID_RESPONSE, parse_from_remote_cleanup, TAG,
                    "data.fetchInterval is not valid");
  state->fetchInterval = values->valueint;
  if (state->fetchInterval < 5) {
    ESP_LOGW(TAG, "Fetch interval was less than 5. Setting to 5.");
    state->fetchInterval = 5;
  }

parse_from_remote_cleanup:
  cJSON_Delete(json);
  return ret;
}

esp_err_t state_fetch_remote(state_handle_t state) {
  ESP_LOGI(TAG, "Fetching remote state");
  esp_err_t ret = ESP_OK;
  fetch_ctx_handle_t ctx;

  ESP_GOTO_ON_ERROR(fetch_init(&ctx), state_fetch_remote_cleanup, TAG,
                    "Error initiating the request context");

  ctx->url = CONFIG_ENDPOINT_STATE_URL;
  ctx->method = HTTP_METHOD_GET;

  ESP_GOTO_ON_ERROR(fetch_perform(ctx), state_fetch_remote_cleanup, TAG,
                    "Error fetching data from endpoint");

  ESP_GOTO_ON_FALSE(ctx->response->status_code < 300, ESP_ERR_INVALID_RESPONSE,
                    state_fetch_remote_cleanup, TAG,
                    "Invalid response status code \"%d\"",
                    ctx->response->status_code);

  ESP_GOTO_ON_ERROR(
      parse_from_remote(state, ctx->response->data, ctx->response->length),
      state_fetch_remote_cleanup, TAG,
      "Invalid JSON response or content length");

state_fetch_remote_cleanup:
  fetch_end(ctx);
  return ret;
}