#include "esp_common.h"
#include "freertos/task.h"
#include "gpio.h"
#include "self_test.h"
#include "freertos/semphr.h"

int last_reed_close_time = 0;
//self test flags
bool reed_status = TRUE;
bool lcd_status = TRUE;
bool gps_status = TRUE;
bool cadence_status = TRUE;

//semaphore for blocking measuring tasks before self tests(4 tasks) are complete
xSemaphoreHandle self_test_semaphore;

/******************************************************************************
 * FunctionName : user_rf_cal_sector_set
 * Description  : SDK just reversed 4 sectors, used for rf init data and paramters.
 *                We add this function to force users to set rf cal sector, since
 *                we don't know which sector is free in user's application.
 *                sector map for last several sectors : ABCCC
 *                A : rf cal
 *                B : rf init data
 *                C : sdk parameters
 * Parameters   : none
 * Returns      : rf cal sector
*******************************************************************************/
uint32 user_rf_cal_sector_set(void)
{
    flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;
    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256:
            rf_cal_sec = 128 - 5;
            break;

        case FLASH_SIZE_8M_MAP_512_512:
            rf_cal_sec = 256 - 5;
            break;

        case FLASH_SIZE_16M_MAP_512_512:
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
        case FLASH_SIZE_32M_MAP_1024_1024:
            rf_cal_sec = 1024 - 5;
            break;

        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;
}
/******************************************************************************
 * FunctionName : task_blinker
 * Description  : test task. Blink built in LED on GPIO_2 on nodeMCU. 
 *                If it blinks, system is still working xD
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void task_blinker(void* ignore)
{
    printf("[Blinker] Starting\n");
    printf("[Blinker] Configuring GPIOS\n");
    gpio16_output_conf();
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U,FUNC_GPIO2);
    while(true) {
        vTaskDelay(1000/portTICK_RATE_MS);
        gpio_output_conf(0, BIT2, BIT2, 0);
    	gpio16_output_set(1);
        vTaskDelay(1000/portTICK_RATE_MS);
        gpio16_output_set(0);
        gpio_output_conf(BIT2, 0, BIT2, 0);
    }
    vTaskDelete(NULL);
}

void print_last_reed_time(void* parameter)
{
    printf("[ReedPrinter] Starting\n");
    for( int i = 0;i<100;i++ ){
        last_reed_close_time++;
        printf("[ReedPrinter] %d\n",last_reed_close_time);
        vTaskDelay(1000/portTICK_RATE_MS);
    }
    printf("Ending ReedPrinter\n");
    vTaskDelete( NULL );
}

/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void user_init(void)
{
    printf("SDK version:%s\n", system_get_sdk_version());
    printf("Starting OBC. Tasks running: %d\n",uxTaskGetNumberOfTasks());
    //init self test semaphore
    printf("Starting self test...\n");
    self_test_semaphore =  xSemaphoreCreateCounting( 4, 0 );
    //start self_tests. Since saetup have priority 1, use priority 0 to block execution of setup until semaphore is released
    xTaskCreate(&fake_reed_tester,"fake_reed_tester",2048,(void*)&reed_status,0,NULL);
    xTaskCreate(&fake_cadence_tester,"fake_cadence_tester",2048,(void*)&cadence_status,0,NULL);
    xTaskCreate(&fake_lcd_tester,"fake_lcd_tester",2048,(void*)&lcd_status,0,NULL);
    xTaskCreate(&fake_gps_tester,"fake_gps_tester",2048,(void*)&gps_status,0,NULL);
    //now wait for semaphore realease
    for(int i= 0; i< 4; i++){
        xSemaphoreTake(self_test_semaphore, portMAX_DELAY);
    }
    printf("Self Test complete. Initializing\n");
    if(gps_status == FALSE) {
        printf("GPS not detected, OBC tracking function are offline\n");
    }
    //store tasks handlers for later
    xTaskHandle task_blinker_handle;
    xTaskHandle reed_printer_handle;

    xTaskCreate(&task_blinker, "task_blinker", 2048, NULL, 1, &task_blinker_handle);
    printf("task_blinker started with priority %d\n",uxTaskPriorityGet(task_blinker_handle));

    //start fake reed counter task, assign a global int to it
    xTaskCreate(&print_last_reed_time,"reed_printer",2048,(void*)&last_reed_close_time,2,&reed_printer_handle);
    printf("reed_printer started with priority %d\n",uxTaskPriorityGet(reed_printer_handle));

    //@TODO display some timings info. For fun.
    printf("Startup ended. Tasks running: %d\n",uxTaskGetNumberOfTasks());
}

