#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "screen_pcd8544.h"
#include "obc.h"
#include "pcd8544.h"

#include "pcd8544_font_utils.h"

#define TAG "PCD8544_TASK"

//hardware setup
pcd8544_config_t config = {
        .spi_host = HSPI_HOST,
        .is_backlight_common_anode = false,
};

// screen number to render
uint8_t screenNumber = 1;
// number of renders before screen toggle
uint8_t frameCounter = 0;

/**
 * Used to display current speed compared to average as bar on the right side of the screen
 * */
static void drawAverageBar() {
    // calculate ratio in percents
    int16_t percentHeight;
    float diff = rideParams.avgSpeed - rideParams.speed;
    if (diff < 0) {
        percentHeight = (int)((diff) / (rideParams.avgSpeed > 0 ? rideParams.avgSpeed : 1.00) * 100);
    } else {
        percentHeight = (int)((diff) / (rideParams.speed > 0 ? rideParams.speed : 1.00) * 100);
    }    
    // R = [-100, 100]    
    percentHeight = percentHeight > 100 ? 100 : percentHeight;
    percentHeight = percentHeight < -100 ? -100 : percentHeight;
    // render
    for (int i = 0; i < 2; i++) {
        pcd8544_draw_line(82 + i, 24 + (24 * percentHeight / 100), 82 + i, 48);
    }
}

/**
 * @warning screen designed for default font
 * */
static void drawMainScreen() {
    uint8_t charRowsArr[6];
    uint8_t *speedChars[4];
    uint8_t *distanceChars[6];
    uint8_t currentDrawingPos = 0;
    // draw speed using big chars
    vGetSpeedChars(speedChars, &rideParams.speed, charRowsArr);
    for (int i = 0; i < 4; i++) {
        pcd8544_set_pos(currentDrawingPos, 0);
        pcd8544_draw_bitmap(speedChars[i], charRowsArr[i], 3, true);
        currentDrawingPos += charRowsArr[i];
    }

    pcd8544_draw_line(0, 24, 84, 24);

    // and draw distance
    currentDrawingPos = 0;
    vGetDistanceChars(distanceChars, &rideParams.distance, charRowsArr);
    for (int i = 0; i < 6; i++) {
        if (distanceChars[i] == 0) {
            break;
        }
        pcd8544_set_pos(currentDrawingPos, 3);
        pcd8544_draw_bitmap(distanceChars[i], charRowsArr[i], 3, true);
        currentDrawingPos += charRowsArr[i];
    }

    drawAverageBar();
    pcd8544_finalize_frame_buf();
    // display some units
    // prevents screen from breaking, there will be no space on it anyways
    if(rideParams.distance < 100) {
        pcd8544_set_pos(currentDrawingPos + 2, 5);
        pcd8544_printf("km");
    }
    
    pcd8544_set_pos(53, 2); //speed render has constant size
    pcd8544_printf("km/h");
    pcd8544_sync_and_gc();

}
// FIXME add better line clearing
static void drawSimpleDetailsScreen() {
    uint8_t rideHours, rideMinutes, rideSeconds;

    rideHours = rideParams.totalRideTimeMs / 3600000;
    rideMinutes = (rideParams.totalRideTimeMs / 60000) - (rideHours*60);
    rideSeconds = (rideParams.totalRideTimeMs / 1000) - (rideMinutes*60);

    //TODO add mutual exclusion for reading rideParams?
    pcd8544_set_pos(0, 0);
    pcd8544_puts("              ");
    pcd8544_set_pos(0, 0);
    pcd8544_printf("AvgSpd: %0.2f", rideParams.avgSpeed);
    pcd8544_set_pos(0, 1);
    pcd8544_printf("T.Dstn: %0.2f", rideParams.totalDistance);
    pcd8544_set_pos(0, 2);
    pcd8544_printf("MaxSpd: %0.2f", rideParams.maxSpeed);
    pcd8544_set_pos(0, 3);
    pcd8544_puts("              ");
    pcd8544_set_pos(0, 3);
    pcd8544_printf("Time: %d:%d:%d", rideHours, rideMinutes, rideSeconds);
    pcd8544_sync_and_gc();
}

// render one of the screens, use bitwise flags
// uses crude method for calculating time, depending on the consants and number of rendered frames since last screen toggle
// @warning each screen method must sync buffer for itself
static void screenRenderer() {
    
    // @TODO fix trans_queue_size reaches PCD8544_TRANS_QUEUE_SIZE(32). Is this library issue of blokcing task mid-render or something? 
    if (SCREEN_CHANGE_AFTER_MS / REFRESH_RATE_MS <= frameCounter) {
        frameCounter = 0;
        screenNumber++;
        if (screenNumber > IMPLEMENTED_SCREENS) {
            screenNumber = 1;
        }
    }
    pcd8544_clear_display();
    pcd8544_finalize_frame_buf();
    pcd8544_sync_and_gc();

    switch (screenNumber) {
        case 1:
            drawMainScreen();
            break;
        case 2:
            drawSimpleDetailsScreen();
            break;
        default:
            drawMainScreen();
            ESP_LOGW(TAG, "[Renderer] Screen %d doesn't exists!", screenNumber);
            break;
    }
   
    
    frameCounter++;
}

void vScreenRefreshTask(void* data) {
    // TODO enable screen toggler and rendering on not moving, before powerdown state
    ESP_LOGI(TAG, "Init");

    bool screenOff = false;
    uint32_t notificationVal = 0;
    TickType_t xLastWakeTime;

    screenRenderer();

    while(true) {
        if (rideParams.moving) {
            xLastWakeTime = xTaskGetTickCount();
            // refresh screen when riding
            vTaskDelayUntil(&xLastWakeTime, REFRESH_RATE_MS/portTICK_RATE_MS);
            screenRenderer();
        } else {
            // unblock and refresh/turn on screen when started moving again
            notificationVal = ulTaskNotifyTake(pdTRUE, POWER_SAVE_DELAY_MS/portTICK_RATE_MS);
            // turn off screen after POWER_SAVE_DELAY_MS
            if (notificationVal == 0 && !screenOff) {
                ESP_LOGI(TAG, "Screen powerdown mode");
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

void vInitPcd8544Screen() {
    ESP_LOGI(TAG, "Init pcd8544 screen");
    pcd8544_init(&config);
    pcd8544_set_backlight(true);
    pcd8544_clear_display();
    pcd8544_finalize_frame_buf();
    pcd8544_sync_and_gc();
}
