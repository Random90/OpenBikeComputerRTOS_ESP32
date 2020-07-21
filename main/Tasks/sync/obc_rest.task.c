#include "esp_http_client.h"
#include "esp_tls.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_event_base.h"
#include <string.h>
#include "freertos/task.h"

#include "wifi.h"
#include "obc.h"

#define WEB_SERVER CONFIG_OBC_SERVER_ADDR
#define WEB_PORT CONFIG_OBC_SERVER_PORT

#define MAX_HTTP_RECV_BUFFER 512
#define MAX_HTTP_OUTPUT_BUFFER 2048


#define TAG "OBC_REST"

static esp_err_t vhttpEventHandler(esp_http_client_event_t *evt)
{
    static char *output_buffer;  // Buffer to store response of http request from event handler
    //static int output_len;       // Stores number of bytes read
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            if (output_buffer != NULL) {
                // Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
                // ESP_LOG_BUFFER_HEX(TAG, output_buffer, output_len);
                free(output_buffer);
                output_buffer = NULL;
            }
            //output_len = 0;
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            int mbedtls_err = 0;
            esp_err_t err = esp_tls_get_and_clear_last_error(evt->data, &mbedtls_err, NULL);
            if (err != 0) {
                if (output_buffer != NULL) {
                    free(output_buffer);
                    output_buffer = NULL;
                }
                //output_len = 0;
                ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
                ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
            }
            break;
    }
    return ESP_OK;
}

void vHttpSyncRest(void *pvParameters)
{
    //@TODO delay sync startup
    //@TODO retry after delay on no wifi
    vInitWifiStation();

    char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};

    esp_http_client_config_t config = {
        .host = WEB_SERVER,
        .port = WEB_PORT,
        .path = "/",
        .query = "",
        .event_handler = vhttpEventHandler,
        .user_data = local_response_buffer,        // Pass address of local buffer to get response
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    // GET
    esp_http_client_set_method(client, HTTP_METHOD_GET);
    esp_http_client_set_header(client, "X-Requested-With", "OBC");
    esp_http_client_set_header(client, "accept", "application/json");
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }
    ESP_LOG_BUFFER_HEX(TAG, local_response_buffer, strlen(local_response_buffer));

    esp_http_client_cleanup(client);
    vDeinitWifiStation();

    ESP_LOGI(TAG, "Finish http example");
    vTaskDelete(NULL);
}

static void vRideStopEventHandler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data) {
    xTaskCreate(&vHttpSyncRest, "vHttpSyncRest", 8192, NULL, 5, NULL);
}

void vInitSync() {
    ESP_LOGI(TAG, "Init");
    ESP_ERROR_CHECK(esp_event_handler_register_with(obc_events_loop, OBC_EVENTS, RIDE_STOP_EVENT, vRideStopEventHandler, obc_events_loop));
}
