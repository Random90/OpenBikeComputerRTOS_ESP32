#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/spi_common.h"
#include "sdkconfig.h"
#include "esp_log.h"

#include "settings.h"
#include "obc.h"

#include "utils/math.h"
#include "pcd8544.h"

#include "Tasks/screen_pcd8544/screen_pcd8544.h"
#include "Tasks/storage/spiffs_main.h"






//hardware setup
pcd8544_config_t config = {
        .spi_host = HSPI_HOST,
        .is_backlight_common_anode = false,
};
// RTOS specific variables
static xQueueHandle reed_evt_queue = NULL;
static const char* TAG = "OBC_MAIN";

// OBC global ride params
ride_params_t rideParams = {
    .moving = false, 
    .rotations = 0,
    .prevRotationTickCount = 0,
    .totalRideTimeMs = 0,
    .speed = 0.0,
    .avgSpeed = 0.0,
    .distance = 0.0,
};

// Handles for the tasks create by init.
static TaskHandle_t screenRefreshTask = NULL;

void vBlinkerTask(void *pvParameter)
{
    portTickType xLastWakeTime;

    ESP_LOGI(TAG, "Initialize blinking");

    /* specify that the function of a given pin 
    should be that of GPIO as opposed to some other function 
    */
    gpio_pad_select_gpio(BLINK_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
    while(true) {
        xLastWakeTime = xTaskGetTickCount();
        vTaskDelayUntil(&xLastWakeTime,1000/portTICK_RATE_MS);
        gpio_set_level(BLINK_GPIO, 0);
        vTaskDelayUntil(&xLastWakeTime,1000/portTICK_RATE_MS);
        gpio_set_level(BLINK_GPIO, 1);
    }
}
/* 
TODO maybe use simple timer instead of the task to avoid preemption 
*/
static void vRideStatusIntervalCheckTask(void *arg) {
    int msg_count;
    TickType_t currentTickCount;
    int timeInactive;

    ESP_LOGI(TAG, "Init ride status watchdog");

    while(true) {
        msg_count = uxQueueMessagesWaitingFromISR(reed_evt_queue);
        currentTickCount = xTaskGetTickCount();
        timeInactive = ((int) currentTickCount - (int) rideParams.prevRotationTickCount) * (int) portTICK_RATE_MS;

        if(rideParams.moving && !msg_count && timeInactive > RIDE_TIMEOUT_MS) {
            ESP_LOGI(TAG, "[RIDE_STATUS] Stopped moving");
            rideParams.speed = 0.0;
            rideParams.msBetweenRotationTicks = 0;
            rideParams.moving = false;
            rideParams.prevRotationTickCount = 0;
            // TODO add semaphore and run data saving, stop screen refresh, run powersave algorithms, sync with could etc
        } 
        vTaskDelayUntil(&currentTickCount, 1000/portTICK_RATE_MS);
    }
}

static void vCalcRideParamsOnISRTask(void* data)
{
    ESP_LOGI(TAG, "Init reed switch");

    for(;;) {
        if(xQueueReceive(reed_evt_queue, &rideParams.rotationTickCount, portMAX_DELAY)) {
            // TODO create buffer for reed time impulses before calculating time and speed
            // TODO block rideParams while calculating?
            if (rideParams.prevRotationTickCount != 0) {
                rideParams.moving = true;
                rideParams.rotations++;
                rideParams.msBetweenRotationTicks = ((int) rideParams.rotationTickCount - (int) rideParams.prevRotationTickCount) * (int) portTICK_RATE_MS;
                rideParams.totalRideTimeMs += rideParams.msBetweenRotationTicks;
                rideParams.speed = ( (float) CIRCUMFERENCE/1000000 ) / ( (float) rideParams.msBetweenRotationTicks / 3600000 ); //km/h
                rideParams.distance = (float)rideParams.rotations * (float)CIRCUMFERENCE/1000000;
                rideParams.avgSpeed = rideParams.distance / ( (float) rideParams.totalRideTimeMs / 3600000 );
            }
            rideParams.prevRotationTickCount = rideParams.rotationTickCount;    

            ESP_LOGI(TAG, "[REED] count: %d, speed: %0.2f, diff: %d, distance: %0.2f", rideParams.rotations, rideParams.speed, rideParams.msBetweenRotationTicks, rideParams.distance);
            ESP_LOGI(TAG, "[AVGS] speed: %0.2f", rideParams.avgSpeed);
            // inform screen refresh task about movement
            xTaskNotifyGive(screenRefreshTask);
        }
    }
}

//IRAM_ATTR - function with this will be moved to RAM in order to execute faster than default from flash
static void IRAM_ATTR vReedISR(void* arg) {
    portTickType xLastReedTickCount = xTaskGetTickCount();
    xQueueSendFromISR(reed_evt_queue, &xLastReedTickCount, NULL);
}

void vInitPcd8544Screen() {
    ESP_LOGI(TAG, "Init pcd8544 screen");
    pcd8544_init(&config);
    pcd8544_set_backlight(true);
    pcd8544_clear_display();
    pcd8544_finalize_frame_buf();
    pcd8544_sync_and_gc();
}

void vInitTasks() {
    xTaskCreate(&vBlinkerTask, "vBlinkerTask", 2048, NULL, 5, NULL);
    xTaskCreate(&vRideStatusIntervalCheckTask, "vRideStatusIntervalCheckTask", 2048, NULL, 3, NULL);
    xTaskCreate(&vCalcRideParamsOnISRTask, "vCalcRideParamsOnISRTask", 2048, NULL, 2, NULL);  
    xTaskCreate(&vScreenRefreshTask, "vScreenRefreshTask", 2048, NULL, 2, &screenRefreshTask);
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

void app_main()
{
    ESP_LOGI(TAG, "Initializing");
    vInitSpiffs();
    vInitPcd8544Screen();
    vAttachInterrupts();
    vInitTasks();
    ESP_LOGI(TAG, "Startup complete");
}
