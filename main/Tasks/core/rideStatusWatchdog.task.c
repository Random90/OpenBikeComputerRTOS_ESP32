#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_event_base.h"

#include "rideStatusWatchdog.task.h"
#include "settings.h"
#include "obc.h"

#define TAG "RIDE_WATCHDOG"
/* 
TODO maybe use simple timer instead of the task to avoid preemption 
Task which checks every second if bike is still moving, by reading number of
items waiting in reed ISR queue. Calls notification
*/
void vRideStatusWatchdogTask(void *arg) {
    int msg_count;
    TickType_t currentTickCount;
    int timeInactive;

    ESP_LOGI(TAG, "Initialization");

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

            // store ride params if stopped
            xTaskNotifyGive(spiffsSyncOnStopTaskHandle);
            esp_event_post_to(obc_events_loop, OBC_EVENTS, RIDE_STOP_EVENT, NULL, 0, portMAX_DELAY);
        } 
        vTaskDelayUntil(&currentTickCount, 1000/portTICK_RATE_MS);
    }
}