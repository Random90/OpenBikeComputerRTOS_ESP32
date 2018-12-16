#include "self_test.h"
#include "esp_common.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
//its all fake! :)
extern xSemaphoreHandle self_test_semaphore;
void fake_reed_tester(void* status) {
    //we have reed connected! impossible (without additional curcuit) to check in real life, but OK!
    vTaskDelay(200/portTICK_RATE_MS);
    bool is_working = *((bool *) status);
    is_working = TRUE;
    printf("[selfTest] Reed switch is OK\n");
    //release the samophore, my work here is done!
    xSemaphoreGive(self_test_semaphore);
    vTaskDelete( NULL );
}

void fake_cadence_tester(void* status) 
{
    vTaskDelay(300/portTICK_RATE_MS);
    bool is_working = *((bool *) status);
    is_working = TRUE;
    printf("[selfTest] Cadence meter is OK\n");
    xSemaphoreGive(self_test_semaphore);
    vTaskDelete( NULL );
}

void fake_lcd_tester(void* status) 
{
    vTaskDelay(1600/portTICK_RATE_MS);
    bool is_working = *((bool *) status);
    is_working = TRUE;
    printf("[selfTest] LCD is OK\n");
    xSemaphoreGive(self_test_semaphore);
    vTaskDelete( NULL );
}

void fake_gps_tester(void* status) 
{
    vTaskDelay(4000/portTICK_RATE_MS);
    bool is_working = *((bool *) status);
    is_working = FALSE;
    printf("[selfTest] GPS module is not connected\n");
    xSemaphoreGive(self_test_semaphore);
    vTaskDelete( NULL );
}
