#include "esp_spiffs.h"
#include "esp_log.h"
#include "esp_err.h"

#include "spiffs_main.h"
#include "obc.h"

#define TAG "SPIFFS_MAIN"

//@TODO saving ride data

TaskHandle_t spiffsSyncOnStopTaskHandle = NULL;

// values read from files
float maxSpeedFileBuff = 0.00;
float totalDistanceFileBuff = 0.00;

void vReadMaxSpeed() {
    FILE *fmaxSpeed = fopen("/spiffs/max_speed", "rb");
    if (fmaxSpeed == NULL) {
        ESP_LOGE(TAG, "Failed to open file max_speed");
        return;
    } else {
        fread(&maxSpeedFileBuff, 1, sizeof(maxSpeedFileBuff), fmaxSpeed);
        fclose(fmaxSpeed);
        ESP_LOGI(TAG, "max speed from file %f", maxSpeedFileBuff);
    }
}

void static vReadTotalDistance() {
    FILE *ftotalDistance = fopen("/spiffs/total_distance", "rb");
    if (ftotalDistance == NULL) {
        ESP_LOGE(TAG, "Failed to open file total_distance");
        return;
    } else {
        fread(&totalDistanceFileBuff, 1, sizeof(totalDistanceFileBuff), ftotalDistance);
        fclose(ftotalDistance);
        ESP_LOGI(TAG, "total distance from file %f", totalDistanceFileBuff);
    }
}

static void vSaveMaxSpeed(float speed) {
    ESP_LOGI(TAG, "[SAVING] maxSpeed: %f", speed);
    FILE *fmaxSpeed = fopen("/spiffs/max_speed", "wb");
    if (fmaxSpeed == NULL) {
        ESP_LOGE(TAG, "Failed to open file max_speed for update");
        return;
    } else {
        fwrite(&speed, 1, sizeof(speed), fmaxSpeed);
        fclose(fmaxSpeed);
        maxSpeedFileBuff = speed;
    }
}

static void vSaveTotalDistance() {
    if(rideParams.totalDistance > totalDistanceFileBuff) {
        ESP_LOGI(TAG, "[SAVING] totalDistance: %f", rideParams.totalDistance);
        FILE *fTotalDistance = fopen("/spiffs/total_distance", "wb");
        if (fTotalDistance == NULL) {
            ESP_LOGE(TAG, "Failed to open file total_distance for update");
            return;
        } else {
            fwrite(&rideParams.totalDistance, 1, sizeof(rideParams.totalDistance), fTotalDistance);
            fclose(fTotalDistance);
            totalDistanceFileBuff = rideParams.totalDistance;
        }
    }
}

static void clearCorruptedData() {
    if (maxSpeedFileBuff > 227.72) {
        ESP_LOGW(TAG, "MaxSpeed corrupted, overwriting");
        maxSpeedFileBuff = 0;
        vSaveMaxSpeed(maxSpeedFileBuff);
    }
}

static void vRideStopEventHandler() {
    xTaskNotifyGive(spiffsSyncOnStopTaskHandle);
}

// update rideParams with max speed, total distance from files before ride
static void vPopulateRideParamsFromStorage()
{
    // @TODO mutex?
    vReadMaxSpeed();
    vReadTotalDistance();
    clearCorruptedData();

    rideParams.globalMaxSpeed = maxSpeedFileBuff;
    rideParams.totalDistance = totalDistanceFileBuff;
}

void vInitSpiffs() {
     ESP_LOGI(TAG, "Initializing SPIFFS");

     esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = MAX_FILES,
      .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);

     if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        return;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }

    vPopulateRideParamsFromStorage();
    ESP_ERROR_CHECK(esp_event_handler_register_with(obc_events_loop, OBC_EVENTS, RIDE_STOP_EVENT, vRideStopEventHandler, NULL));
    xTaskCreate(&vSpiffsSyncOnStopTask, "vSpiffsSyncOnStopTask", 2048, NULL, 3, &spiffsSyncOnStopTaskHandle);

}

void vSpiffsSyncOnStopTask(void* data) {
    while (true)
    {
        ulTaskNotifyTake(pdTRUE, SPIFFS_SYNC_INTERVAL_MS/portTICK_RATE_MS);
        if (rideParams.maxSpeed > maxSpeedFileBuff) {
            vSaveMaxSpeed(rideParams.maxSpeed);
        }
        vSaveTotalDistance();
    }
}