#include <time.h>
#include <sys/time.h>
#include "esp_system.h"
#include "esp_log.h"
#include "esp_sntp.h"

#include "wifi.h"
#include "obc.h"

#define TAG "SNTP"

static void updateRideStartedTimestamp(struct timeval *tv)
{
    rideParams.startedTimestamp = tv->tv_sec;
}

static void obtain_time() {
    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_set_time_sync_notification_cb(updateRideStartedTimestamp);
    sntp_init();

    int retry = 0;
    const int retry_count = 10;
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

void vSntpSyncTask() {
    ESP_LOGI(TAG, "Starting wifi");
        if (vInitWifiStation() != ESP_OK) {
            ESP_LOGI(TAG, "Aborting sync due to connection error");
        } else {
            
            time_t now;
            struct tm timeinfo;
            time(&now);
            ESP_LOGI(TAG, "The current timestamp: %ju", (uintmax_t)now);
            localtime_r(&now, &timeinfo);

             // Is time set? If not, tm_year will be (1970 - 1900).
            if (timeinfo.tm_year < (2016 - 1900)) {
                ESP_LOGI(TAG, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
                obtain_time();
                time(&now);
                localtime_r(&now, &timeinfo);
                vDeinitWifiStation();
            }

            char strftime_buf[64];

            strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
            ESP_LOGI(TAG, "The current date/time UTC is: %s. Timestamp: %ju", strftime_buf, (uintmax_t)now);
        }

    vTaskDelete(NULL);
}

