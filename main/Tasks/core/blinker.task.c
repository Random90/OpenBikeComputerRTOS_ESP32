#include "blinker.task.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "settings.h"
#define TAG "BLINKER_TASK"

void vBlinkerTask(void *pvParameter) {
    TickType_t xLastWakeTime;

    ESP_LOGI(TAG, "Initialize blinking");

    /* specify that the function of a given pin
    should be that of GPIO as opposed to some other function
    */
    esp_rom_gpio_pad_select_gpio(BLINK_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
    while (true) {
        xLastWakeTime = xTaskGetTickCount();
        vTaskDelayUntil(&xLastWakeTime, 1000 / portTICK_PERIOD_MS);
        gpio_set_level(BLINK_GPIO, 0);
        vTaskDelayUntil(&xLastWakeTime, 1000 / portTICK_PERIOD_MS);
        gpio_set_level(BLINK_GPIO, 1);
    }
}