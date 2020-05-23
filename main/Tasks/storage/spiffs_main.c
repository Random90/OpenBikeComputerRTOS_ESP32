#include "esp_spiffs.h"
#include "esp_log.h"
#include "esp_err.h"

#include "spiffs_main.h"
#include "obc.h"

#define TAG "SPIFFS_MAIN"

//@TODO clearing value, some protection agains huge speeds from interference
// values read from files
float maxSpeedFileBuff = 0.00;

void fReadMaxSpeed() {
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
// update rideParams with max speed, total distance from files before ride
static void vPopulateRideParamsFromStorage()
{
    // @TODO mutex?
    fReadMaxSpeed();
    rideParams.maxSpeed = maxSpeedFileBuff;
}

static void vSaveMaxSpeed() {
    if(rideParams.maxSpeed > maxSpeedFileBuff) {
        ESP_LOGI(TAG, "[SAVING] maxSpeed: %f", rideParams.maxSpeed);
        FILE *fmaxSpeed = fopen("/spiffs/max_speed", "wb");
        if (fmaxSpeed == NULL) {
            ESP_LOGE(TAG, "Failed to open file max_speed for update");
            return;
        } else {
            fwrite(&rideParams.maxSpeed, 1, sizeof(rideParams.maxSpeed), fmaxSpeed);
            fclose(fmaxSpeed);
        }
    }
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
    
}

void vSpiffsSyncOnStopTask(void* data) {
    uint32_t status;
    while (true)
    {
        status = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        if (status) {
            vSaveMaxSpeed();
        }
    }
}