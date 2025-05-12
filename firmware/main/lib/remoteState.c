
#include "remoteState.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"
#include "lib/cjson/cJSON.h"
#include <string.h>

static const char *TAG = "REMOTE_STATE";

esp_err_t remoteStateInit(RemoteStateHandle *remoteStateHandle) {
  // allocate the the state
  RemoteStateHandle remoteState =
      (RemoteStateHandle)malloc(sizeof(RemoteState));

  remoteState->isDevMode = false;
  remoteState->devModeEndpoint = NULL;

  // pass back the state
  *remoteStateHandle = remoteState;

  return ESP_OK;
}

esp_err_t remoteStateParse(RemoteStateHandle remoteState, char *data,
                           size_t length) {
  esp_err_t ret = ESP_OK;
  const cJSON *values = NULL;
  cJSON *json = cJSON_ParseWithLength(data, length);

  ESP_GOTO_ON_FALSE(json != NULL, ESP_ERR_INVALID_RESPONSE,
                    remoteStateParse_cleanup, TAG,
                    "Invalid JSON response or content length");

  ESP_GOTO_ON_FALSE(cJSON_IsObject(json), ESP_ERR_INVALID_RESPONSE,
                    remoteStateParse_cleanup, TAG,
                    "JSON response is not an object");

  values = cJSON_GetObjectItemCaseSensitive(json, "isDevMode");
  ESP_GOTO_ON_FALSE(cJSON_IsBool(values), ESP_ERR_INVALID_RESPONSE,
                    remoteStateParse_cleanup, TAG,
                    "data.isDevMode is not a boolean");

  remoteState->isDevMode = cJSON_IsTrue(values);

  values = cJSON_GetObjectItemCaseSensitive(json, "devModeEndpoint");
  ESP_GOTO_ON_FALSE(cJSON_IsString(values) && values->valuestring != NULL,
                    ESP_ERR_INVALID_RESPONSE, remoteStateParse_cleanup, TAG,
                    "data.devModeEndpoint is not a string");
  ESP_GOTO_ON_FALSE(strlen(values->valuestring) > 0, ESP_ERR_INVALID_RESPONSE,
                    remoteStateParse_cleanup, TAG,
                    "data.devModeEndpoint is empty");

  // free the old endpoint if it exists
  if (remoteState->devModeEndpoint != NULL) {
    free(remoteState->devModeEndpoint);
  }
  // allocate the new endpoint
  remoteState->devModeEndpoint =
      (char *)malloc(strlen(values->valuestring) + 1);
  strcpy(remoteState->devModeEndpoint, values->valuestring);

remoteStateParse_cleanup:
  cJSON_Delete(json);
  return ret;
}