#ifndef NO_SCREEN
#include "Tasks/screen_pcd8544/screen_pcd8544.h"
#include "driver/spi_common.h"
#endif

#include <stdio.h>
#include <stdlib.h>

#include "Tasks/core/calcRideParamsOnISR.task.h"
#include "Tasks/core/rideStatusWatchdog.task.h"
#include "Tasks/storage/spiffs_main.h"
#include "Tasks/sync/obc_rest.task.h"
#include "Tasks/sync/sntp.task.h"
#include "driver/gpio.h"
#include "esp_event_base.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "obc.h"
#include "sdkconfig.h"
#include "settings.h"
#include "utils/math.h"

#define TAG "OBC_MAIN"

// obc event loop
ESP_EVENT_DEFINE_BASE(OBC_EVENTS);
esp_event_loop_handle_t obc_events_loop;
esp_event_loop_args_t obc_events_loop_args = {
    .queue_size = 5,
    .task_name = "obc_events_loop_task",
    .task_priority = 7,
    .task_stack_size = 2048,
    .task_core_id = tskNO_AFFINITY};

// declare global queues
QueueHandle_t reed_evt_queue = NULL;

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
    .globalMaxSpeed = 0.0,
    .totalDistance = 0.0,
    .startedTimestamp = 0};

// IRAM_ATTR - function with this will be moved to RAM in order to execute faster than default from flash
static void IRAM_ATTR vReedISR(void* arg) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    TickType_t xLastReedTickCount = xTaskGetTickCountFromISR();

    xQueueSendFromISR(reed_evt_queue, &xLastReedTickCount, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

void vInitTasks() {
    xTaskCreate(&vCalcRideParamsOnISRTask, "vCalcRideParamsOnISRTask", 2048, NULL, 6, NULL);
    xTaskCreate(&vRideStatusWatchdogTask, "vRideStatusIntervalCheckTask", 2048, NULL, 3, NULL);
    xTaskCreate(&vSntpSyncTask, "vSntpSyncTask", 4096, NULL, 3, NULL);
    vRegisterServerSyncTask();
}

void vAttachInterrupts() {
    ESP_LOGI(TAG, "Attaching reed switch interrupt");
    // create queue for the reed interrupt
    reed_evt_queue = xQueueCreate(10, sizeof(uint32_t));

    // configure reed switch gpio
    gpio_config_t io_conf;
    io_conf.mode = GPIO_MODE_INPUT;
    // bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = 1ULL << REED_IO_NUM;
    io_conf.pull_up_en = 1;
    io_conf.pull_down_en = 0;
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    // configure GPIO with the given settings
    gpio_config(&io_conf);
    // gpio_set_intr_type(REED_IO_NUM, GPIO_INTR_NEGEDGE);
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    gpio_isr_handler_add(REED_IO_NUM, vReedISR, NULL);
}

void vInitNVS() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

void testHandler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data) {
    ESP_LOGI(TAG, "handling %s %" PRIu32, base, id);
}

void app_main() {
    ESP_LOGI(TAG, "Initializing");
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    // event loop created with task with priority 7
    ESP_ERROR_CHECK(esp_event_loop_create(&obc_events_loop_args, &obc_events_loop));

    vInitNVS();
    vInitSpiffs();
    vAttachInterrupts();
    vInitTasks();
#ifndef NO_SCREEN
    vInitPcd8544Screen();
#endif

    ESP_ERROR_CHECK(esp_event_handler_register_with(obc_events_loop, OBC_EVENTS, ESP_EVENT_ANY_ID, testHandler, obc_events_loop));
    ESP_LOGI(TAG, "Startup complete");
}
