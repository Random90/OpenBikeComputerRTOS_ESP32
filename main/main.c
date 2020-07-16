#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "driver/gpio.h"
#include "driver/spi_common.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"

#include "settings.h"
#include "obc.h"

#include "utils/math.h"

#include "Tasks/core/blinker.task.h"
#include "Tasks/core/rideStatusWatchdog.task.h"
#include "Tasks/core/calcRideParamsOnISR.task.h"
#include "Tasks/screen_pcd8544/screen_pcd8544.h"
#include "Tasks/storage/spiffs_main.h"

#define TAG "OBC_MAIN"

// @TODO move wifi functions somewhere else
#define WIFI_SSID      CONFIG_ESP_WIFI_SSID
#define WIFI_PASSWD      CONFIG_ESP_WIFI_PASSWORD
#define WIFI_MAX_RETRY  CONFIG_ESP_MAXIMUM_RETRY

// declare global queues 
xQueueHandle reed_evt_queue = NULL;

// declare OBC global ride params
ride_params_t rideParams = {
    .moving = false, 
    .rotations = 0,
    .prevRotationTickCount = 0,
    .totalRideTimeMs = 0,
    .speed = 0.0,
    .avgSpeed = 0.0,
    .distance = 0.0,
    .maxSpeed = 0.0,
    .totalDistance = 0.0
};

// Handles for the shared tasks create by init.
TaskHandle_t screenRefreshTaskHandle = NULL;
TaskHandle_t spiffsSyncOnStopTaskHandle = NULL;

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static int s_retry_num = 0;

//IRAM_ATTR - function with this will be moved to RAM in order to execute faster than default from flash
static void IRAM_ATTR vReedISR(void* arg) {
    portTickType xLastReedTickCount = xTaskGetTickCount();
    xQueueSendFromISR(reed_evt_queue, &xLastReedTickCount, NULL);
}

void vInitTasks() {
    xTaskCreate(&vCalcRideParamsOnISRTask, "vCalcRideParamsOnISRTask", 2048, NULL, 6, NULL);  
    xTaskCreate(&vBlinkerTask, "vBlinkerTask", 2048, NULL, 5, NULL);
    xTaskCreate(&vRideStatusWatchdogTask, "vRideStatusIntervalCheckTask", 2048, NULL, 3, NULL);
    xTaskCreate(&vSpiffsSyncOnStopTask, "vSpiffsSyncOnStopTask", 2048, NULL, 3, &spiffsSyncOnStopTaskHandle);
    xTaskCreate(&vScreenRefreshTask, "vScreenRefreshTask", 2048, NULL, 2, &screenRefreshTaskHandle);
}

void vAttachInterrupts() {
    ESP_LOGI(TAG, "Attaching reed switch interrupt");
    // create queue for the reed interrupt
    reed_evt_queue = xQueueCreate(10, sizeof(uint32_t));

    // configure reed switch gpio
    gpio_config_t io_conf;
    io_conf.mode = GPIO_MODE_INPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = 1ULL<<REED_IO_NUM;
    io_conf.pull_up_en = 1;
    io_conf.intr_type = GPIO_PIN_INTR_POSEDGE;
    //configure GPIO with the given settings
    gpio_config(&io_conf);
    gpio_set_intr_type(REED_IO_NUM, GPIO_INTR_POSEDGE);
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    gpio_isr_handler_add(REED_IO_NUM, vReedISR, (void*) REED_IO_NUM);
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < WIFI_MAX_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:%s", ip4addr_ntoa(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void vWifiInitStation(void)
{
    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA INIT");
    s_wifi_event_group = xEventGroupCreate();
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWD,
	        .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false
            }
        }
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);
    
     if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap %s", WIFI_SSID);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to %s,", WIFI_SSID);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED WIFI ERROR");
    }

    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler));
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler));
    vEventGroupDelete(s_wifi_event_group);
}

void vInitNVS() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

void app_main()
{
    ESP_LOGI(TAG, "Initializing");
    vInitNVS();
    vInitSpiffs();
    // @TODO init wifi after ride?
    vWifiInitStation();
    vInitPcd8544Screen();
    vAttachInterrupts();
    vInitTasks();
    ESP_LOGI(TAG, "Startup complete");
}
