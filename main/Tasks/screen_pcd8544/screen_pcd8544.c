#include "screen_pcd8544.h"
#include "freertos/task.h"
#include "pcd8544_font_utils.h"
static const char* TAG = "PCD8544_TASK";
bool bToogle = false;
// FIXME add better line clearing
static void printDataToScreen() {
    //TODO add mutual exclusion for reading rideParams?
    // pcd8544_set_pos(0, 0);
    // pcd8544_puts("              ");
    // pcd8544_set_pos(0, 0);
    // pcd8544_printf("Speed: %0.2f",  rideParams.speed);
    // pcd8544_set_pos(0, 1);
    // pcd8544_printf("Distance: %0.2f",  rideParams.distance);
    // pcd8544_set_pos(0, 2);
    // pcd8544_printf("Rotations:");
    // pcd8544_set_pos(0, 3);
    // pcd8544_printf("%d",  rideParams.rotations);
    // pcd8544_set_pos(0, 4);
    // pcd8544_puts("              ");
    // pcd8544_set_pos(0, 4);
    // pcd8544_printf("DiffMS: %d",  rideParams.msBetweenRotationTicks);
    // // toggle to visualize screen refresh if not moving
    // bToogle = !bToogle;
    // pcd8544_set_pos(0, 5);
    // bToogle ? pcd8544_printf("/") : pcd8544_printf("\\");
    // pcd8544_sync_and_gc();
    struct two_digits_ptrs *chars = getChars(10);
    pcd8544_clear_display();
    pcd8544_draw_bitmap(chars->first, 16, 3, false);
    pcd8544_set_pos(30, 3);
    pcd8544_draw_bitmap(chars->second, 16, 3, false);
    pcd8544_finalize_frame_buf();
    pcd8544_sync_and_gc();
}

void vScreenRefreshTask(void* data) {
    bool screenOff = false;
    uint32_t notificationVal = 0;
    ESP_LOGI(TAG, "Init");
    printDataToScreen();
    while(true) {
        TickType_t xLastWakeTime = xTaskGetTickCount();
       
        if (rideParams.moving) {
            // refresh screen when riding
            vTaskDelayUntil(&xLastWakeTime, REFRESH_RATE_MS/portTICK_RATE_MS);
            printDataToScreen();
        } else {
            // unblock and refresh/turn on screen when started moving again
            notificationVal = ulTaskNotifyTake(pdTRUE, POWER_SAVE_DELAY_MS/portTICK_RATE_MS);
            // turn off screen after POWER_SAVE_DELAY_MS
            if (notificationVal == 0 && !screenOff) {
                pcd8544_set_powerdown_mode(true);
                screenOff = true;
            } else if (notificationVal > 0 && screenOff) {
                pcd8544_set_powerdown_mode(false);
                // TODO trans_queue_size warning
                screenOff = false;
            }
        }
    }
}