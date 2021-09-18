#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "screen_pcd8544.h"
#include "obc.h"
#include "pcd8544.h"

#include "pcd8544_font_utils.h"

#define TAG "PCD8544_TASK"

/**
 * Array of screen delays in miliseconds for each existing screen
 * Refresh rate is 1/500ms, so delay for each screen should be multiplication of that
 **/
static const int SCREEN_TIMINGS[2] = {8000, 3500};

static TaskHandle_t powerdownScreenTaskHandle = NULL;

//hardware setup
pcd8544_config_t config = {
        .spi_host = VSPI_HOST,
        .is_backlight_common_anode = false,
};

// screen control variables

bool screenPowerdownMode = false;
TickType_t lastScreenChange = 0;
// screen number to render
uint8_t currentScreenIdx = 1;

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

    rideHours = rideParams.totalRideTimeMs / 3600000 % 60;
    rideMinutes = rideParams.totalRideTimeMs / 60000 % 60;
    rideSeconds = rideParams.totalRideTimeMs / 1000 % 60;

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
    pcd8544_printf("Time: %02d:%02d:%02d", rideHours, rideMinutes, rideSeconds);
    pcd8544_sync_and_gc();
}

// timebased screen switcher function
// @warning each screen method must sync buffer for itself
static void screenRenderer() {
    TickType_t currentTickCount = xTaskGetTickCount();
    int timeInactive = ((int) currentTickCount - (int) lastScreenChange) * (int) portTICK_RATE_MS;
    
    if(timeInactive >= SCREEN_TIMINGS[currentScreenIdx]) {
        lastScreenChange = xTaskGetTickCount();
        currentScreenIdx++;
        if (currentScreenIdx > IMPLEMENTED_SCREENS - 1) {
            currentScreenIdx = 0;
        }
    }

    pcd8544_clear_display();
    pcd8544_finalize_frame_buf();
    pcd8544_sync_and_gc();

    switch (currentScreenIdx) {
        case 0:
            drawMainScreen();
            break;
        case 1:
            drawSimpleDetailsScreen();
            break;
        default:
            drawMainScreen();
            ESP_LOGW(TAG, "[Renderer] Screen %d doesn't exists!", currentScreenIdx);
            break;
    }
}

void vPowerdownScreenTask(void* data) {
    if (ulTaskNotifyTake(pdTRUE, POWER_SAVE_DELAY_MS/portTICK_RATE_MS) == 0) {
        ESP_LOGI(TAG, "Screen powerdown mode enabled");
        pcd8544_set_powerdown_mode(true);
        screenPowerdownMode = true;
    } else {
        ESP_LOGI(TAG, "Aborting screen powerdown mode");
    }
    vTaskDelete(NULL);
} 

static void vRideStartEventHandler() {
    if (powerdownScreenTaskHandle) {
        xTaskNotifyGive(powerdownScreenTaskHandle);
    }
    if (screenPowerdownMode) {
        pcd8544_set_powerdown_mode(false);
        screenPowerdownMode = false;
    }
}

static void vRideStopEventHandler() {
    xTaskCreate(&vPowerdownScreenTask, "vPowerdownScreenTask", 2048, NULL, 2, &powerdownScreenTaskHandle);
}

void vScreenRefreshTask(void* data) {
    ESP_LOGI(TAG, "Screen refresher started");
    TickType_t xLastWakeTime;

    ESP_ERROR_CHECK(esp_event_handler_register_with(obc_events_loop, OBC_EVENTS, RIDE_STOP_EVENT, vRideStopEventHandler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register_with(obc_events_loop, OBC_EVENTS, RIDE_START_EVENT, vRideStartEventHandler, NULL));

    vRideStopEventHandler();
    screenRenderer();

    while(true) {
        xLastWakeTime = xTaskGetTickCount();
        vTaskDelayUntil(&xLastWakeTime, REFRESH_RATE_MS/portTICK_RATE_MS);

        if (!screenPowerdownMode) {
            screenRenderer();
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

    xTaskCreate(&vScreenRefreshTask, "vScreenRefreshTask", 2048, NULL, 2, NULL);
}