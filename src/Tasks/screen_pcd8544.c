#include "screen_pcd8544.h"
#include "freeRTOS/task.h"
#include "pcd8544.h"


#define REFRESH_RATE_MS 500
void vScreenRefresh(void* data) {
    printf("[OBC] Init screen refresh task\n");

    while(true) {
        TickType_t xLastWakeTime = xTaskGetTickCount();
        
        vTaskDelayUntil(&xLastWakeTime, REFRESH_RATE_MS/portTICK_RATE_MS);
        // TODO add mutual exclusion for reading rideParams?
        pcd8544_set_pos(0, 4);
        pcd8544_printf("%d %0.2f %0.2f", rideParams.rotations, rideParams.speed, rideParams.distance);
        pcd8544_sync_and_gc();
        printf("[PCD] count: %d, speed: %0.2f, diff: %d, distance: %0.2f\n", rideParams.rotations, rideParams.speed, rideParams.msBetweenRotationTicks, rideParams.distance);     

    }

}