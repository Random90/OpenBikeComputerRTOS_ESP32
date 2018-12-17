#include "self_test.h"
#include "esp_common.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
//its all fake! :)
extern xSemaphoreHandle self_test_semaphore;
extern bool reed_status;
extern bool lcd_status;
extern bool gps_status;
extern bool cadence_status;
void fake_reed_tester(void* ignore) {
    //we have reed connected! impossible (without additional curcuit) to check in real life, but OK!
    vTaskDelay(200/portTICK_RATE_MS);
    reed_status = TRUE;
    printf("[selfTest] Reed switch is OK\n");
    //release the samophore, my work here is done!
    xSemaphoreGive(self_test_semaphore);
    vTaskDelete( NULL );
}

void fake_cadence_tester(void* ignore) 
{
    vTaskDelay(300/portTICK_RATE_MS);
    cadence_status = TRUE;
    printf("[selfTest] Cadence meter is OK\n");
    xSemaphoreGive(self_test_semaphore);
    vTaskDelete( NULL );
}

void fake_lcd_tester(void* ignore) 
{
    vTaskDelay(1600/portTICK_RATE_MS);
    lcd_status = TRUE;
    printf("[selfTest] LCD is OK\n");
    xSemaphoreGive(self_test_semaphore);
    vTaskDelete( NULL );
}

void fake_gps_tester(void* ignore) 
{
    vTaskDelay(4000/portTICK_RATE_MS);
    gps_status = FALSE;
    printf("[selfTest] GPS module is not connected\n");
    xSemaphoreGive(self_test_semaphore);
    vTaskDelete( NULL );
}
