#include <string.h>

#include "esp_event_base.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_tls.h"
#include "freertos/task.h"
#include "lwip/sys.h"
#include "obc.h"
#include "settings.h"
#include "wifi.h"

#define WEB_SERVER CONFIG_OBC_SERVER_ADDR
#define WEB_PORT CONFIG_OBC_SERVER_PORT
#define API_KEY CONFIG_OBC_SERVER_API_KEY

#define MAX_HTTP_RECV_BUFFER 512
#define MAX_HTTP_OUTPUT_BUFFER 2048

#define TAG "OBC_REST"

static TaskHandle_t httpSyncRestTaskHandle = NULL;
static bool sync_pending = false;
static bool reqestSuccessful;

static esp_err_t vhttpEventHandler(esp_http_client_event_t *evt) {
    static char *output_buffer;  // Buffer to store response of http request from event handler
    // static int output_len;       // Stores number of bytes read
    switch (evt->event_id) {
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
            // output_len = 0;
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
                // output_len = 0;
                ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
                ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
            }
            break;
        case HTTP_EVENT_REDIRECT:
            ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
            esp_http_client_set_header(evt->client, "From", "OBC_ESP32");
            esp_http_client_set_header(evt->client, "Accept", "text/html");
            esp_http_client_set_redirection(evt->client);
            break;
    }
    return ESP_OK;
}
static bool vPerformPostReq() {
    char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};
    char post_data[100];

    esp_http_client_config_t config = {
        .host = WEB_SERVER,
        .path = "/activities/",
        .query = "",
        .event_handler = vhttpEventHandler,
        .user_data = local_response_buffer,  // Pass address of local buffer to get response
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    snprintf(
        post_data,
        sizeof(post_data),
        "{\"circumference\": %d, \"rotations\": %" PRIu32 ", \"rideTime\": %" PRIu32 ", \"started\": %ju}",
        CIRCUMFERENCE,
        rideParams.rotations,
        rideParams.totalRideTimeMs,
        (uintmax_t)rideParams.startedTimestamp);
    // @TODO use params and config
    esp_http_client_set_url(client, "http://malina9.ddns.net/obc_server/activities/");
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "X-Requested-With", "OBC_ESP32");
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "Authorization", API_KEY);
    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    ESP_LOGI(TAG, "Performing HTTP post request to OBC_Server");

    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        reqestSuccessful = true;
        ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %" PRIu64,
                 esp_http_client_get_status_code(client),
                 esp_http_client_get_content_length(client));
    } else {
        reqestSuccessful = false;
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }
    ESP_LOG_BUFFER_HEX(TAG, local_response_buffer, strlen(local_response_buffer));

    esp_http_client_cleanup(client);
    return reqestSuccessful;
}

static void vHttpSyncRestTask(void *pvParameters) {
    // if unblocking happens by timeout (== 0) - sync, otherwise abort
    if (ulTaskNotifyTake(pdTRUE, OBC_SERVER_SYNC_DEBOUNCE_MS / portTICK_PERIOD_MS) == 0) {
        ESP_LOGI(TAG, "Starting wifi");
        if (vInitWifiStation() != ESP_OK) {
            ESP_LOGI(TAG, "Aborting sync due to connection error");
        } else {
            esp_event_post_to(obc_events_loop, OBC_EVENTS, SYNC_START_EVENT, NULL, 0, portMAX_DELAY);
            vPerformPostReq();
            esp_event_post_to(obc_events_loop, OBC_EVENTS, SYNC_STOP_EVENT, &reqestSuccessful, sizeof(reqestSuccessful), portMAX_DELAY);
            vDeinitWifiStation();
            ESP_LOGI(TAG, "Sync finished");
        }
    } else {
        ESP_LOGI(TAG, "Activity detected, aborting synchronization");
    }
    sync_pending = false;
    vTaskDelete(NULL);
}

static void vRideStopEventHandler(void *handler_args, esp_event_base_t base, int32_t id, void *event_data) {
    if (rideParams.distance < OBC_SERVER_SYNC_MIN_DIST) {
        ESP_LOGI(TAG, "Minimum distance requirement for synchronization not met, aborting");
        return;
    }
    if (!sync_pending) {
        sync_pending = true;
        xTaskCreate(&vHttpSyncRestTask, "vHttpSyncRest", 8192, NULL, 5, &httpSyncRestTaskHandle);
    } else {
        ESP_LOGW(TAG, "Synchronization already pending");
    }
}

static void vRideStartEventHandler(void *handler_args, esp_event_base_t base, int32_t id, void *event_data) {
    if (sync_pending) {
        xTaskNotifyGive(httpSyncRestTaskHandle);
    }
}

void vRegisterServerSyncTask() {
    ESP_LOGI(TAG, "Init");
    ESP_ERROR_CHECK(esp_event_handler_register_with(obc_events_loop, OBC_EVENTS, RIDE_STOP_EVENT, vRideStopEventHandler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register_with(obc_events_loop, OBC_EVENTS, RIDE_START_EVENT, vRideStartEventHandler, NULL));
}
