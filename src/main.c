/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

/* Can run 'make menuconfig' to choose the GPIO to blink,
   or you can edit the following line and set a number here.
*/
#define BLINK_GPIO CONFIG_BLINK_GPIO

void blink_task(void *pvParameter)
{
    /* Configure the IOMUX register for pad BLINK_GPIO (some pads are
       muxed to GPIO on reset already, but some default to other
       functions and need to be switched to GPIO. Consult the
       Technical Reference for a list of pads and their default
       functions.)
    */
    gpio_pad_select_gpio(BLINK_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
    while(1) {
        /* Blink off (output low) */
        gpio_set_level(BLINK_GPIO, 0);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        /* Blink on (output high) */
        gpio_set_level(BLINK_GPIO, 1);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void app_main()
{
    xTaskCreate(&blink_task, "blink_task", configMINIMAL_STACK_SIZE, NULL, 5, NULL);
}

// #include "esp_common.h"
// #include "freertos/task.h"
// #include "gpio.h"
// #include "self_test.h"
// #include "freertos/semphr.h"
// #include "settings.h"

// #define ETS_GPIO_INTR_ENABLE() _xt_isr_unmask(1 << ETS_GPIO_INUM) //enable interrupt by disabling specific cpu mask
// #define ETS_GPIO_INTR_DISABLE() _xt_isr_mask(1 << ETS_GPIO_INUM) //disable interrupt

// //reed switch settings
// #define REED_ISR_DBC_TIME 50 //debouce in millis @TODO calculate optimal max time(bike cant ride 200kmh, can it?)
// #define REED_IO_MUC PERIPHS_IO_MUX_MTCK_U
// #define REED_IO_NUM 13
// #define REED_IO_FUNC FUNC_GPIO13
// #define REED_IO_PIN GPIO_Pin_13 //pin 13 is D7 on nodeMCU
// //speed calculate
// //(float(CIRCUMFERENCE)/1000000)/((float(time) - float(last_magnet_time))/3600000); //km/h
// //interrupt globals @TODO use queues to change values?
// static volatile uint32 last_reed_close_time = 0; //miliseconds since startup
// static volatile uint32 reed_closed_number = 0;
// /**@brief handle interrupts
//  * @todo ithis should be on falling edge, but is it?
//  */
// void io_intr_handler(void) {
//     uint32 status = GPIO_REG_READ(GPIO_STATUS_ADDRESS); //read status of interrupt
//     uint32 isr_start_time = system_get_time() / 1000;
//     //reed switch
//     if (status & REED_IO_PIN) {
//         if((isr_start_time - last_reed_close_time) > REED_ISR_DBC_TIME) {
//             reed_closed_number++;
//             //just for debug, remove printf from ISR or move it to special ISR_ECHO task
//             //printf("[ISR][Reed]: started %d, last %d, now %d\n", isr_start_time, last_reed_close_time, system_get_time() / 1000); 
//         }
//     }    
//     //@TODO isr startup or ending time here?
//     last_reed_close_time = isr_start_time;
//     GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, status); //CLEAR THE STATUS IN THE W1 INTERRUPT REGISTER
// }

// //self test flags
// bool reed_status = TRUE;
// bool lcd_status = TRUE;
// bool gps_status = TRUE;
// bool cadence_status = TRUE;

// //semaphore for blocking measuring tasks before self tests(4 tasks) are complete
// xSemaphoreHandle self_test_semaphore;

// //must be here :(
// uint32 user_rf_cal_sector_set(void)
// {
//     flash_size_map size_map = system_get_flash_size_map();
//     uint32 rf_cal_sec = 0;
//     switch (size_map) {
//         case FLASH_SIZE_4M_MAP_256_256:
//             rf_cal_sec = 128 - 5;
//             break;

//         case FLASH_SIZE_8M_MAP_512_512:
//             rf_cal_sec = 256 - 5;
//             break;

//         case FLASH_SIZE_16M_MAP_512_512:
//         case FLASH_SIZE_16M_MAP_1024_1024:
//             rf_cal_sec = 512 - 5;
//             break;

//         case FLASH_SIZE_32M_MAP_512_512:
//         case FLASH_SIZE_32M_MAP_1024_1024:
//             rf_cal_sec = 1024 - 5;
//             break;

//         default:
//             rf_cal_sec = 0;
//             break;
//     }

//     return rf_cal_sec;
// }
// //@brief blinker will blink using two LEDS - test if everything is working real time
// void task_blinker(void* ignore)
// {
    
//     portTickType xLastWakeTime;
//     printf("[Blinker] Starting\n");
//     printf("[Blinker] Configuring GPIOS\n");
//     gpio16_output_conf();
//     PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U,FUNC_GPIO2);
//     xLastWakeTime = xTaskGetTickCount();
//     while(true) {
//         vTaskDelayUntil(&xLastWakeTime,1000/portTICK_RATE_MS);
//         gpio_output_conf(0, BIT2, BIT2, 0);
//     	gpio16_output_set(1);
//         vTaskDelayUntil(&xLastWakeTime,1000/portTICK_RATE_MS);
//         gpio16_output_set(0);
//         gpio_output_conf(BIT2, 0, BIT2, 0);
//     }
//     vTaskDelete(NULL);
// }
// /*
// Every 5 seconds print to serial number of reed switch closed times
// */
// void print_last_reed_time(void* parameter)
// {
//     portTickType xLastWakeTime;
//     printf("[ReedPrinter] Starting\n");
//     xLastWakeTime = xTaskGetTickCount();
//     for( ;; ){
//         printf("[ReedPrinter] reed closed %d times\n",reed_closed_number);
//         vTaskDelayUntil(&xLastWakeTime, 5000/portTICK_RATE_MS);
//     }
//     printf("Ending ReedPrinter!\n");
//     vTaskDelete( NULL );
// }

// /******************************************************************************
//  * FunctionName : user_init
//  * Description  : entry of user application, init user function here
//  * Parameters   : none
//  * Returns      : none
// *******************************************************************************/
// void user_init(void)
// {
//     printf("SDK version:%s\n", system_get_sdk_version());
//     printf("Starting OBC. Tasks running: %d\n",uxTaskGetNumberOfTasks());
//     //init self test semaphore
//     printf("Starting self test...\n");
//     //
//     self_test_semaphore = xSemaphoreCreateCounting(1,0);
//     //start self_tests. Since setup have priority 1, use priority 2 to block execution of setup until semaphore is released
//     //edit: that priorioty teory is not holding water. Semaphore blocks execution of setup regardless of prority used for task running.
//     // i wonder though why binarySemaphore is not blocking execution of setup
//     //setup is probably not a task
//     xTaskCreate(&self_test,"self_test_1time_task",176,NULL,2,NULL);
//     //now wait for semaphore realease
//     xSemaphoreTake(self_test_semaphore, portMAX_DELAY);
    
//     printf("Self Test complete. Initializing\n");
//     printf("Attaching reed switch interrupt\n");
//     GPIO_ConfigTypeDef io_in_conf;
//     io_in_conf.GPIO_IntrType = GPIO_PIN_INTR_NEGEDGE;
//     io_in_conf.GPIO_Mode = GPIO_Mode_Input;
//     io_in_conf.GPIO_Pin = REED_IO_PIN;
//     io_in_conf.GPIO_Pullup = GPIO_PullUp_EN;
//     gpio_config(&io_in_conf);
//     gpio_intr_handler_register(io_intr_handler, NULL);
//     ETS_GPIO_INTR_ENABLE();

//     if(gps_status == FALSE) {
//         printf("GPS not detected, OBC tracking functions are offline\n");
//     }
//     //store tasks handlers for later
//     xTaskHandle task_blinker_handle;
//     xTaskHandle reed_printer_handle;

//     xTaskCreate(&task_blinker, "task_blinker", 512, NULL, 1, &task_blinker_handle);
//     printf("task_blinker started with priority %d\n",uxTaskPriorityGet(task_blinker_handle));

//     //start reed printer for testing purposes only
//     xTaskCreate(&print_last_reed_time,"reed_printer",512,(void*)&last_reed_close_time,2,&reed_printer_handle);
//     printf("reed_printer started with priority %d\n",uxTaskPriorityGet(reed_printer_handle));

//     //IMPORTANT NOTE: task_blinker has lower priority than reed printer, but it is still able to run, because of vTaskDelay used in both tasks.
//     // If print_last_reed_time is blocked by delay, CPU has free ticks for lower priority tasks
//     //@TODO display some timings info. For fun.
//     printf("Startup ended. Tasks running: %d\n",uxTaskGetNumberOfTasks());
// }

