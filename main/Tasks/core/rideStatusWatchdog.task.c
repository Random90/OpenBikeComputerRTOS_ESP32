#include "rideStatusWatchdog.task.h"

#include <stdint.h>

#include "esp_event_base.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "obc.h"
#include "settings.h"

#define TAG "RIDE_WATCHDOG"
/*
Task which checks every second if bike is still moving, by reading number of
items waiting in reed ISR queue. Posts to obc_event_loop with ride_stop_event
*/
void vRideStatusWatchdogTask(void *arg) {
    int msg_count;
    TickType_t currentTickCount;
    int timeInactive;

    ESP_LOGI(TAG, "Initialization");

    while (true) {
        msg_count = uxQueueMessagesWaitingFromISR(reed_evt_queue);
        currentTickCount = xTaskGetTickCount();
        timeInactive = ((int)currentTickCount - (int)rideParams.prevRotationTickCount) * (int)portTICK_PERIOD_MS;

        if (rideParams.moving && !msg_count && timeInactive > RIDE_TIMEOUT_MS) {
            ESP_LOGI(TAG, "[RIDE_STATUS] Stopped moving");
            rideParams.speed = 0.0;
            rideParams.moving = false;
            rideParams.prevRotationTickCount = 0;

            esp_event_post_to(obc_events_loop, OBC_EVENTS, RIDE_STOP_EVENT, NULL, 0, portMAX_DELAY);
        }
        vTaskDelayUntil(&currentTickCount, 1000 / portTICK_PERIOD_MS);
    }
}