#include "screen_pcd8544.h"
#include "freertos/task.h"
#include "pcd8544_font_utils.h"
static const char* TAG = "PCD8544_TASK";

bool bToogle = false;
uint8_t screenFlags = 0b00000001; // initial value, main screen enabled

/**
 * Used to display current speed compared to average as bar on the right side of the screen
 * */
static void drawAverageBar(uint8_t percentHeight) {
    for (int i = 0; i < 2; i++) {
        pcd8544_draw_line(82 + i, 0 + (48 * percentHeight / 100), 82 + i, 48);
    }
}

/**
 * @warning screen designed for default font
 * */
static void drawMainScreen() {
    pcd8544_clear_display();
    uint8_t charRowsArr[6];
    uint8_t currentDrawingPos = 0;
    // draw speed using big chars
    uint8_t **speedChars = getSpeedChars(&rideParams.speed, charRowsArr);
    for (int i = 0; i < 4; i++) {
        pcd8544_set_pos(currentDrawingPos, 0);
        pcd8544_draw_bitmap(speedChars[i], charRowsArr[i], 3, true);
        currentDrawingPos += charRowsArr[i];
    }

    pcd8544_draw_line(0, 24, 84, 24);

    // and draw distance
    currentDrawingPos = 0;
    uint8_t **distanceChars = getDistanceChars(&rideParams.distance, charRowsArr);
    for (int i = 0; i < 6; i++) {
        if (distanceChars[i] == NULL) {
            break;
        }
        pcd8544_set_pos(currentDrawingPos, 3);
        pcd8544_draw_bitmap(distanceChars[i], charRowsArr[i], 3, true);
        currentDrawingPos += charRowsArr[i];
    }

    drawAverageBar(60);
    pcd8544_finalize_frame_buf();
    // display some units
    // prevents screen from breaking, there will be no space on it anyways
    if(rideParams.distance < 100) {
        pcd8544_set_pos(currentDrawingPos + 2, 5);
    }
    pcd8544_printf("km");
    pcd8544_set_pos(53, 2); //speed render has constant size
    pcd8544_printf("km/h");
    pcd8544_sync_and_gc();
}
// FIXME add better line clearing
static void drawSimpleDetailsScreen() {
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
    // toggle to visualize screen refresh if not moving
    bToogle = !bToogle;
    pcd8544_set_pos(0, 5);
    bToogle ? pcd8544_printf("/") : pcd8544_printf("\\");
    pcd8544_sync_and_gc();
}

// render one of the screens, use bitwise flags
static void screenRenderer(TickType_t *lastWakeTime) {
    // @TODO fix trans_queue_size reaches PCD8544_TRANS_QUEUE_SIZE(32). Is this library issue of blokcing task mid-render or something? 
    // ESP_LOGI(TAG, "Renderer last wake time %d", (int)lastWakeTime);
    drawMainScreen();
}

void vScreenRefreshTask(void* data) {
    bool screenOff = false;
    uint32_t notificationVal = 0;
    ESP_LOGI(TAG, "Init");

    TickType_t xLastWakeTime;
    drawMainScreen();

    while(true) {
        // @TODO display speed 0.00 before screen saver
        if (rideParams.moving) {
            xLastWakeTime = xTaskGetTickCount();
            // refresh screen when riding
            screenRenderer(&xLastWakeTime);
            vTaskDelayUntil(&xLastWakeTime, REFRESH_RATE_MS/portTICK_RATE_MS);
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