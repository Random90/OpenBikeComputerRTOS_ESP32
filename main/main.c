#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/spi_common.h"

#include "sdkconfig.h"
#include "settings.h"
#include "obc.h"

#include "Tasks/screen_pcd8544.h"
#include <u8g2.h>
#include "u8g2_esp32_hal.h"


//hardware setup
// TODO move to ext file
// pcd8544 screen
#define PCD_PIN_CLK 14
#define PCD_PIN_MOSI 12 // DIN
#define PCD_PIN_RESET 4
#define PCD_PIN_DC 25
#define PCD_PIN_CS 15
#define PCD_PIN_BL 16
u8g2_t u8g2;

// RTOS specific variables
static xQueueHandle reed_evt_queue = NULL;

// OBC global ride params
ride_params_t rideParams = {
    .moving = false, 
    .rotations = 0,
    .prevRotationTickCount = 0,
    .speed = 0.0,
    .distance = 0.0,
};
void vBlinkerTask(void *pvParameter)
{
    portTickType xLastWakeTime;

    printf("[OBC] Initialize blinking\n");

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
/* 
TODO maybe use simple timer instead of the task to avoid preemption 
*/
static void vRideStatusIntervalCheckTask(void *arg) {
    int msg_count;
    TickType_t currentTickCount;
    int timeInactive;

    printf("[OBC] Starting ride status watchdog task\n");

    while(true) {
        msg_count = uxQueueMessagesWaitingFromISR(reed_evt_queue);
        currentTickCount = xTaskGetTickCount();
        timeInactive = ((int) currentTickCount - (int) rideParams.prevRotationTickCount) * (int) portTICK_RATE_MS;

        if(rideParams.moving && !msg_count && timeInactive > RIDE_TIMEOUT_MS) {
            printf("[RIDE_STATUS] Stopped moving\n");
            rideParams.speed = 0.0;
            rideParams.msBetweenRotationTicks = 0;
            rideParams.moving = false;
            // TODO add semaphore and run data saving, stop screen refresh, run powersave algorithms, sync with could etc
        } 
        vTaskDelayUntil(&currentTickCount, 1000/portTICK_RATE_MS);
    }
}

static void vCalcRideParamsOnISRTask(void* data)
{
    printf("[OBC] Start reed switch task!\n");

    for(;;) {
        if(xQueueReceive(reed_evt_queue, &rideParams.rotationTickCount, portMAX_DELAY)) {
            // TODO create buffer for reed time impulses before calculating time and speed
            // TODO block rideParams while calculating?
            rideParams.moving = true;
            rideParams.rotations++;
            rideParams.msBetweenRotationTicks = ((int) rideParams.rotationTickCount - (int) rideParams.prevRotationTickCount) * (int) portTICK_RATE_MS;
            rideParams.speed = ( (float) CIRCUMFERENCE/1000000 ) / ( (float) rideParams.msBetweenRotationTicks / 3600000 ); //km/h
            rideParams.distance = (float)rideParams.rotations * (float)CIRCUMFERENCE/1000000;
            rideParams.prevRotationTickCount = rideParams.rotationTickCount;    
            printf("[REED] count: %d, speed: %0.2f, diff: %d, distance: %0.2f\n", rideParams.rotations, rideParams.speed, rideParams.msBetweenRotationTicks, rideParams.distance);        
        }
    }
}

//IRAM_ATTR - function with this will be moved to RAM in order to execute faster than default from flash
static void IRAM_ATTR vReedISR(void* arg) {
    portTickType xLastReedTickCount = xTaskGetTickCount();
    xQueueSendFromISR(reed_evt_queue, &xLastReedTickCount, NULL);
}

void vInitPcd8544Screen() {
    printf("[OBC] Init pcd8544 screen\n");
    gpio_pad_select_gpio(PCD_PIN_BL);
    gpio_set_direction(PCD_PIN_BL, GPIO_MODE_OUTPUT);
    gpio_set_level(PCD_PIN_BL, 1);
    u8g2_esp32_hal_t u8g2_esp32_hal = U8G2_ESP32_HAL_DEFAULT;
	u8g2_esp32_hal.clk   = PCD_PIN_CLK;
	u8g2_esp32_hal.mosi  = PCD_PIN_MOSI;
	u8g2_esp32_hal.cs    = PCD_PIN_CS;
	u8g2_esp32_hal.dc    = PCD_PIN_DC;
	u8g2_esp32_hal.reset = PCD_PIN_RESET;
	u8g2_esp32_hal_init(u8g2_esp32_hal);

    u8g2_Setup_pcd8544_84x48_f(&u8g2, U8G2_R0, u8x8_byte_4wire_sw_spi, u8g2_esp32_gpio_and_delay_cb);
    u8g2_InitDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0);
    u8g2_ClearBuffer(&u8g2);
    u8g2_SetFont(&u8g2, u8g2_font_ncenB14_tr);
    u8g2_DrawBox(&u8g2, 10,20, 20, 30);
    u8g2_DrawStr(&u8g2, 0,15,"Hello World!");
    u8g2_SendBuffer(&u8g2);
    //TODO backlight usage?
}

void vInitTasks() {
    xTaskCreate(&vBlinkerTask, "vBlinkerTask", 2048, NULL, 5, NULL);
    xTaskCreate(&vRideStatusIntervalCheckTask, "vRideStatusIntervalCheckTask", 2048, NULL, 3, NULL);
    xTaskCreate(&vCalcRideParamsOnISRTask, "vCalcRideParamsOnISRTask", 2048, NULL, 2, NULL);  
    xTaskCreate(&vScreenRefreshTask, "vScreenRefreshTask", 2048, NULL, 1, NULL);
}

void vAttachInterrupts() {
    printf("[OBC] Attaching reed switch interrupt\n");
    // create queue for the reed interrupt
    reed_evt_queue = xQueueCreate(10, sizeof(uint32_t));

    // configure reed switch gpio
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
    gpio_isr_handler_add(REED_IO_NUM, vReedISR, (void*) REED_IO_NUM);
}

void app_main()
{
    // TODO use ESP_LOGI?
    printf("[OBC] IDF version: %s\n",esp_get_idf_version());
    printf("[OBC] Initializing \n");
    vInitPcd8544Screen();
    vAttachInterrupts();
    vInitTasks();
    printf("[OBC] Init reed queue\n");
    printf("[OBC] Startup complete \n");
}
