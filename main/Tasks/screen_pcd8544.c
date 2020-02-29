#include "screen_pcd8544.h"
#include "freertos/task.h"
// FIXME add better line clearing
void vScreenRefreshTask(void* data) {
    printf("[OBC] Init screen refresh task\n");

    while(true) {
        TickType_t xLastWakeTime = xTaskGetTickCount();
        
        vTaskDelayUntil(&xLastWakeTime, REFRESH_RATE_MS/portTICK_RATE_MS);
        //TODO add mutual exclusion for reading rideParams?
        pcd8544_set_pos(0, 0);
        pcd8544_puts("              ");
        pcd8544_set_pos(0, 0);
        pcd8544_printf("Speed: %0.2f",  rideParams.speed);
        pcd8544_set_pos(0, 1);
        pcd8544_printf("Distance: %0.2f",  rideParams.distance);
        pcd8544_set_pos(0, 2);
        pcd8544_printf("Rotations:");
        pcd8544_set_pos(0, 3);
        pcd8544_printf("%d",  rideParams.rotations);
        pcd8544_set_pos(0, 4);
        pcd8544_puts("              ");
        pcd8544_set_pos(0, 4);
        pcd8544_printf("DiffMS: %d",  rideParams.msBetweenRotationTicks);
        pcd8544_sync_and_gc();
    }
}