
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

#include "calcRideParamsOnISR.task.h"
#include "settings.h"
#include "obc.h"

#define TAG "RIDE_CALC"

void vCalcRideParamsOnISRTask(void* data)
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
                // TODO sometimes tick count doesn't update even with queue? prevent speed = inf for now
                if (rideParams.msBetweenRotationTicks > 0.00) {
                    rideParams.speed = ( (float) CIRCUMFERENCE/1000000 ) / ( (float) rideParams.msBetweenRotationTicks / 3600000 ); //km/h
                }
                if (rideParams.speed > rideParams.maxSpeed) {
                    rideParams.maxSpeed = rideParams.speed;
                }
                rideParams.distance = (float)rideParams.rotations * (float)CIRCUMFERENCE/1000000;

                rideParams.avgSpeed = rideParams.distance / ( (float) rideParams.totalRideTimeMs / 3600000 );
            }
            rideParams.prevRotationTickCount = rideParams.rotationTickCount;    

            ESP_LOGI(TAG, "[REED] count: %d, speed: %0.2f, diff: %d, distance: %0.2f", rideParams.rotations, rideParams.speed, rideParams.msBetweenRotationTicks, rideParams.distance);
            ESP_LOGI(TAG, "[AVGS] speed: %0.2f", rideParams.avgSpeed);
            // inform screen refresh task about movement
            xTaskNotifyGive(screenRefreshTaskHandle);
        }
    }
}