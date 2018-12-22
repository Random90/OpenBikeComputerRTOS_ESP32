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
void self_test(void* ignore) {
    //fake delay
    vTaskDelay(100/portTICK_RATE_MS);
    lcd_status = TRUE;
    printf("[selfTest] LCD is OK\n");

    vTaskDelay(1000/portTICK_RATE_MS);
    gps_status = FALSE;
    printf("[selfTest] GPS module is not connected\n");
    vTaskDelay(100/portTICK_RATE_MS);
    xSemaphoreGive(self_test_semaphore);
    vTaskDelete( NULL );
}