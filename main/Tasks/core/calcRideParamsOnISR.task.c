
#include "calcRideParamsOnISR.task.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "obc.h"
#include "settings.h"

#define TAG "RIDE_CALC"

bool ignoreReed = false;

// handle server sychronization start event and set flags to block data
// during synchronization, calculations should be blocked
static void vOnSyncStartHandle(void *handler_args, esp_event_base_t base, int32_t id, void *event_data) {
    ignoreReed = true;
}

// handle server sychronization start event and set flags to block data
// if synchronization finishes with success, finish the ride, by resseting rideParams, otherwise continue
static void vOnSyncFinishHandle(void *handler_args, esp_event_base_t base, int32_t id, void *event_data) {
    bool reset = *((bool *)event_data);

    if (reset) {
        rideParams.rotations = 0;
        rideParams.totalRideTimeMs = 0;
        rideParams.speed = 0.0;
        rideParams.distance = 0.0;
        rideParams.avgSpeed = 0.0;
        rideParams.prevRotationTickCount = 0.0;
    }

    ignoreReed = false;
}

void vCalcRideParamsOnISRTask(void *data) {
    float currentSpeed;
    uint32_t msBetweenRotationTicks = 0;

    ESP_LOGI(TAG, "Init reed switch");

    ESP_ERROR_CHECK(esp_event_handler_register_with(obc_events_loop, OBC_EVENTS, SYNC_START_EVENT, vOnSyncStartHandle, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register_with(obc_events_loop, OBC_EVENTS, SYNC_STOP_EVENT, vOnSyncFinishHandle, NULL));

    for (;;) {
        if (xQueueReceive(reed_evt_queue, &rideParams.rotationTickCount, portMAX_DELAY) && !ignoreReed) {
            if (rideParams.rotationTickCount == rideParams.prevRotationTickCount || rideParams.rotationTickCount == 0) {
                continue;
            };

            msBetweenRotationTicks = ((int)rideParams.rotationTickCount - (int)rideParams.prevRotationTickCount) * (int)portTICK_PERIOD_MS;

            if (msBetweenRotationTicks <= REED_BOUNCE_MS) {
                continue;
            }

            rideParams.prevRotationTickCount = rideParams.rotationTickCount;
            // ESP_LOGI(TAG, "Reed switch triggered %" PRIu32 " diff: %" PRIu16, rideParams.rotationTickCount, msBetweenRotationTicks);

            // TODO block rideParams while calculating?
            if (!rideParams.moving) {
                esp_event_post_to(obc_events_loop, OBC_EVENTS, RIDE_START_EVENT, NULL, 0, portMAX_DELAY);
            } else {
                // ignore the first tick after ride start, since it is idle time
                rideParams.totalRideTimeMs += msBetweenRotationTicks;
            }

            currentSpeed = ((float)CIRCUMFERENCE / 1000000) / ((float)msBetweenRotationTicks / 3600000);  // km/h
            rideParams.moving = true;
            rideParams.rotations++;
            rideParams.speed = currentSpeed;
            rideParams.distance += (float)CIRCUMFERENCE / 1000000;
            rideParams.totalDistance += (float)CIRCUMFERENCE / 1000000;
            rideParams.avgSpeed = rideParams.distance / ((float)rideParams.totalRideTimeMs / 3600000);
            if (rideParams.speed > rideParams.maxSpeed) {
                rideParams.maxSpeed = rideParams.speed;
            }

            // ESP_LOGI(TAG, "[REED] count: %" PRIu32 ", speed: %0.2f, diff: %" PRIu16 ", distance: %0.2f", rideParams.rotations, rideParams.speed, msBetweenRotationTicks, rideParams.distance);
            // ESP_LOGI(TAG, "[AVGS] speed: %0.2f", rideParams.avgSpeed);
        }
    }
}