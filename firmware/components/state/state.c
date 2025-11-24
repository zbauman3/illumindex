

#include "cJSON.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"
#include <string.h>

#include "network/fetch.h"

#include "state.h"

static const char *TAG = "STATE";

// allocates a state struct and sets the default values
esp_err_t state_init(state_handle_t *state_handle) {
  state_handle_t state = (state_handle_t)malloc(sizeof(state_t));
  if (state == NULL) {
    ESP_LOGE(TAG, "Failed to allocate memory for state");
    return ESP_ERR_NO_MEM;
  }

  state->fetch_interval = CONFIG_ENDPOINT_FETCH_INTERVAL_DEFAULT;
  state->invalid_remote_state = false;
  state->invalid_commands = false;
  state->invalid_wifi_state = false;
  // set to the max so it fetches immediately.
  state->fetch_loop_seconds = 65535;
  state->fetch_failure_count = 0;
  // set to the max so it fetches immediately.
  state->remote_state_seconds = 65535;

  state->command_endpoint =
      (char *)malloc(sizeof(char) * (strlen(CONFIG_ENDPOINT_URL) + 1));
  if (state->command_endpoint == NULL) {
    ESP_LOGE(TAG, "Failed to allocate memory for command endpoint");
    free(state);
    return ESP_ERR_NO_MEM;
  }
  strcpy(state->command_endpoint, CONFIG_ENDPOINT_URL);

  // we don't want to do any more frequent than every 5 seconds.
  if (state->fetch_interval < 5) {
    ESP_LOGW(TAG, "Fetch interval was less than 5. Setting to 5.");
    state->fetch_interval = 5;
  }

  // pass back the state
  *state_handle = state;

  return ESP_OK;
}

void state_end(state_handle_t state) {
  free(state->command_endpoint);
  free(state);
}

// parse the remote state JSON response into the state struct
esp_err_t parse_from_remote(state_handle_t state, char *data, size_t length) {
  esp_err_t ret = ESP_OK;
  // reused for each value extraction
  const cJSON *values = NULL;
  cJSON *json = cJSON_ParseWithLength(data, length);

  ESP_GOTO_ON_FALSE(json != NULL, ESP_ERR_INVALID_RESPONSE,
                    parse_from_remote_cleanup, TAG,
                    "Invalid JSON response or content length");

  ESP_GOTO_ON_FALSE(cJSON_IsObject(json), ESP_ERR_INVALID_RESPONSE,
                    parse_from_remote_cleanup, TAG,
                    "JSON response is not an object");

  values = cJSON_GetObjectItemCaseSensitive(json, "commandEndpoint");
  ESP_GOTO_ON_FALSE(cJSON_IsString(values) && values->valuestring != NULL,
                    ESP_ERR_INVALID_RESPONSE, parse_from_remote_cleanup, TAG,
                    "data.commandEndpoint is not a string");
  ESP_GOTO_ON_FALSE(strlen(values->valuestring) > 0, ESP_ERR_INVALID_RESPONSE,
                    parse_from_remote_cleanup, TAG,
                    "data.commandEndpoint is empty");
  // free the old endpoint if it was set
  free(state->command_endpoint);
  state->command_endpoint = NULL;
  // allocate the new endpoint
  state->command_endpoint = (char *)malloc(strlen(values->valuestring) + 1);
  ESP_GOTO_ON_FALSE(state->command_endpoint != NULL, ESP_ERR_NO_MEM,
                    parse_from_remote_cleanup, TAG,
                    "Failed to allocate memory for commandEndpoint");
  strcpy(state->command_endpoint, values->valuestring);

  values = cJSON_GetObjectItemCaseSensitive(json, "fetchInterval");
  ESP_GOTO_ON_FALSE(cJSON_IsNumber(values), ESP_ERR_INVALID_RESPONSE,
                    parse_from_remote_cleanup, TAG,
                    "data.fetchInterval is not a number");
  ESP_GOTO_ON_FALSE(values->valueint > 0 && values->valueint <= 65535,
                    ESP_ERR_INVALID_RESPONSE, parse_from_remote_cleanup, TAG,
                    "data.fetchInterval is not valid");
  state->fetch_interval = values->valueint;
  if (state->fetch_interval < 5) {
    ESP_LOGW(TAG, "Fetch interval was less than 5. Setting to 5.");
    state->fetch_interval = 5;
  }

parse_from_remote_cleanup:
  cJSON_Delete(json);
  return ret;
}

// fetch the actual remote state from the endpoint and parse it
esp_err_t state_fetch_remote(state_handle_t state) {
  ESP_LOGD(TAG, "Fetching remote state");
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