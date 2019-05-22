#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/spi_common.h"
#include "sdkconfig.h"
#include "pcd8544.h"
#define ESP_INTR_FLAG_DEFAULT 0

#define BLINK_GPIO CONFIG_BLINK_GPIO
#define REED_IO_NUM 18

//hardware setup
pcd8544_config_t config = {
        .spi_host = HSPI_HOST,
        .is_backlight_common_anode = false,
};
// RTOS
static xQueueHandle reed_evt_queue = NULL;

// OBC vars
uint32_t rotations = 0;

void blink_task(void *pvParameter)
{
    portTickType xLastWakeTime;
    printf("[Blinker] Starting\n");
    printf("[Blinker] Configuring GPIOS\n");
    /* specify that the function of a given pin 
    should be that of GPIO as opposed to some other function 
    */
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

static void check_status(void *arg) {
    int msg_count;
    while(true) {
        msg_count = uxQueueMessagesWaitingFromISR(reed_evt_queue);
        printf("[STATUS]queue: %d\n", msg_count);
        vTaskDelay(1000/portTICK_RATE_MS);
    }
}

static void reed_task(void* arg)
{
    int val;
    for(;;) {
        if(xQueueReceive(reed_evt_queue, &val, portMAX_DELAY)) {
            rotations++;
            printf("[REED] %d!\n",rotations);
            pcd8544_set_pos(0, 4);
            pcd8544_printf("%d",rotations);
            pcd8544_sync_and_gc();
        }
    }
}

//IRAM_ATTR - function with this will be moved to RAM in order to execute faster than default from flash
static void IRAM_ATTR io_intr_handler(void* arg) {
    // send GPIO for now, consider sending lastWakeTime?
     uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(reed_evt_queue, &gpio_num, NULL);
}

void app_main()
{
    printf("[OBC] IDF version: %s\n",esp_get_idf_version());
    printf("[OBC] Starting OBC. Tasks running: %d\n",uxTaskGetNumberOfTasks());

    printf("[OBC] Init pcd8544 screen\n");
    
    pcd8544_init(&config);
    pcd8544_set_backlight(true);
    pcd8544_clear_display();
    pcd8544_finalize_frame_buf();
    pcd8544_puts("OpenBikeComputer ESP32");
    pcd8544_set_pos(0, 3);
    pcd8544_puts("Interrupt:");
    pcd8544_sync_and_gc();

    printf("[OBC] Init reed queue\n");
    reed_evt_queue = xQueueCreate(10, sizeof(uint32_t));

    printf("[OBC] Attaching reed switch interrupt\n");
    gpio_config_t io_conf;
    io_conf.mode = GPIO_MODE_INPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = 1ULL<<REED_IO_NUM;
    io_conf.pull_up_en = 1;
    io_conf.intr_type = GPIO_PIN_INTR_POSEDGE;
    //configure GPIO with the given settings
    gpio_config(&io_conf);
    gpio_set_intr_type(REED_IO_NUM, GPIO_INTR_POSEDGE);
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    gpio_isr_handler_add(REED_IO_NUM, io_intr_handler, (void*) REED_IO_NUM);

    printf("[OBC] Initialize blinking!\n");
    xTaskCreate(&blink_task, "blink_task", 2048, NULL, 5, NULL);
    printf("[OBC] Start reed switch task!\n");
    xTaskCreate(&reed_task, "reed_task", 2048, NULL, 1, NULL);
    printf("[OBC] Start status checker task\n");
    xTaskCreate(&check_status, "check_status_task", 2048, NULL, 5, NULL);
    printf("[OBC] OBC startup complete. Tasks running: %d\n",uxTaskGetNumberOfTasks());
}
