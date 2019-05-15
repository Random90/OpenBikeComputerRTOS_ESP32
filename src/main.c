#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

#define ESP_INTR_FLAG_DEFAULT 0

#define BLINK_GPIO CONFIG_BLINK_GPIO
#define REED_IO_NUM 13

void blink_task(void *pvParameter)
{
    /* Configure the IOMUX register for pad BLINK_GPIO (some pads are
       muxed to GPIO on reset already, but some default to other
       functions and need to be switched to GPIO. Consult the
       Technical Reference for a list of pads and their default
       functions.)
    */
    portTickType xLastWakeTime;
    printf("[Blinker] Starting\n");
    printf("[Blinker] Configuring GPIOS\n");
    gpio_pad_select_gpio(BLINK_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
    while(true) {
        xLastWakeTime = xTaskGetTickCount();
        vTaskDelayUntil(&xLastWakeTime,1000/portTICK_RATE_MS);
        gpio_set_level(BLINK_GPIO, 0);
        vTaskDelayUntil(&xLastWakeTime,1000/portTICK_RATE_MS);
        gpio_set_level(BLINK_GPIO, 1);
    }
}

static void IRAM_ATTR io_intr_handler(void* arg) {
    printf("Interrupt");
}

void app_main()
{
    printf("IDF version: %s\n",esp_get_idf_version());
    printf("Starting OBC. Tasks running: %d\n",uxTaskGetNumberOfTasks());
    printf("Attaching reed switch interrupt\n");
    gpio_config_t io_conf;
    io_conf.mode = GPIO_MODE_INPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = 1ULL<<REED_IO_NUM;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 1;
    io_conf.intr_type = GPIO_PIN_INTR_NEGEDGE;
    //configure GPIO with the given settings
    gpio_config(&io_conf);

    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    gpio_isr_handler_add(REED_IO_NUM, io_intr_handler, (void*) REED_IO_NUM);
    
    xTaskCreate(&blink_task, "blink_task", 2048, NULL, 5, NULL);
}
