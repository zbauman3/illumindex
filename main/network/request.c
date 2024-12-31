#include "esp_crt_bundle.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_tls.h"
#include <string.h>
#include <sys/param.h>

#include "esp_http_client.h"

#define MAX_HTTP_RECV_BUFFER 512
#define MAX_HTTP_OUTPUT_BUFFER 2048
static const char *TAG = "NETWORK_REQUEST";

static esp_err_t _http_event_handler(esp_http_client_event_t *evt) {
  static char *output_buffer; // Buffer to store response of http request from
                              // event handler
  static int output_len;      // Stores number of bytes read

  switch (evt->event_id) {
  case HTTP_EVENT_ERROR: {
    ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
    break;
  }
  case HTTP_EVENT_ON_CONNECTED: {
    ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
    break;
  }
  case HTTP_EVENT_HEADER_SENT: {
    ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
    break;
  }
  case HTTP_EVENT_ON_HEADER: {
    ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key,
             evt->header_value);
    break;
  }
  case HTTP_EVENT_REDIRECT: {
    ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
    break;
  }
  case HTTP_EVENT_ON_DATA: {
    ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
    // Clean the buffer in case of a new request
    // if (output_len == 0 && evt->user_data) {
    //   // we are just starting to copy the output data into the use
    //   memset(evt->user_data, 0, MAX_HTTP_OUTPUT_BUFFER);
    // }

    if (output_buffer == NULL) {
      // We initialize output_buffer with 0 because it is used by strlen() and
      // similar functions therefore should be null terminated.
      output_buffer = (char *)calloc(MAX_HTTP_OUTPUT_BUFFER + 1, sizeof(char));
      output_len = 0;
      if (output_buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
        return ESP_FAIL;
      }
    }

    // If user_data buffer is configured, copy the response into the buffer
    int copy_len = 0;

    copy_len = MIN(evt->data_len, (MAX_HTTP_OUTPUT_BUFFER - output_len));
    if (copy_len) {
      memcpy(output_buffer + output_len, evt->data, copy_len);
    }

    output_len += copy_len;

    // if (evt->user_data) {
    // The last byte in evt->user_data is kept for the NULL character in case
    // of out-of-bound access.
    // copy_len = MIN(evt->data_len, (MAX_HTTP_OUTPUT_BUFFER - output_len));
    // if (copy_len) {
    //   memcpy(evt->user_data + output_len, evt->data, copy_len);
    // }
    // } else {
    //   int content_len = 0;
    //   if(esp_http_client_is_chunked_response(evt->client)){
    //     esp_err_t ret = esp_http_client_get_chunk_length(evt->client,
    //     &content_len); ESP_ERROR_CHECK(ret);
    //   }else{
    //     content_len = esp_http_client_get_content_length(evt->client);
    //   }
    //   ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, content_len=%d", content_len);
    //   if (output_buffer == NULL) {
    //     // We initialize output_buffer with 0 because it is used by strlen()
    //     and
    //     // similar functions therefore should be null terminated.
    //     output_buffer = (char *)calloc(content_len + 1, sizeof(char));
    //     output_len = 0;
    //     if (output_buffer == NULL) {
    //       ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
    //       return ESP_FAIL;
    //     }
    //   }
    //   copy_len = MIN(evt->data_len, (content_len - output_len));
    //   if (copy_len) {
    //     memcpy(output_buffer + output_len, evt->data, copy_len);
    //   }
    // output_len += copy_len;
    // }

    break;
  }
  case HTTP_EVENT_ON_FINISH: {
    ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
    if (output_buffer != NULL) {
      // Response is accumulated in output_buffer. Uncomment the below line to
      // print the accumulated response
      ESP_LOGI(TAG, "%s", output_buffer);
      free(output_buffer);
      output_buffer = NULL;
    }
    output_len = 0;
    break;
  }
  case HTTP_EVENT_DISCONNECTED: {
    ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
    int mbedtls_err = 0;
    esp_err_t err = esp_tls_get_and_clear_last_error(
        (esp_tls_error_handle_t)evt->data, &mbedtls_err, NULL);
    if (err != 0) {
      ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
      ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
    }
    if (output_buffer != NULL) {
      free(output_buffer);
      output_buffer = NULL;
    }
    output_len = 0;
    break;
  }
  }
  return ESP_OK;
}

void fetch(void) {
  esp_http_client_config_t config = {
      .url = "https://api-tests-ten.vercel.app/esp/red",
      .timeout_ms = 5000,
      .event_handler = _http_event_handler,
      .crt_bundle_attach = esp_crt_bundle_attach,
  };
  esp_http_client_handle_t client = esp_http_client_init(&config);
  esp_err_t err = esp_http_client_perform(client);

  if (err == ESP_OK) {
    ESP_LOGI(TAG, "HTTPS Status = %d, content_length = %" PRId64,
             esp_http_client_get_status_code(client),
             esp_http_client_get_content_length(client));
  } else {
    ESP_LOGE(TAG, "Error perform http request %s", esp_err_to_name(err));
  }

  esp_http_client_cleanup(client);

  ESP_LOGI(TAG, "Finish http example");
}