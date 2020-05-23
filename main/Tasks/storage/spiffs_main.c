#include "esp_spiffs.h"
#include "esp_log.h"
#include "esp_err.h"

#include "spiffs_main.h"
#include "obc.h"

static const char *TAG = "SPIFFS_MAIN";

// update rideParams with max speed, total distance from files before ride
static void vPopulateRideParamsFromStorage()
{
    FILE *fmaxSpeed = fopen("/spiffs/max_speed.txt", "r");
    if (fmaxSpeed == NULL) {
        ESP_LOGE(TAG, "Failed to open file max_speed.txt");
        return;
    }
    char speedBuff[4];
    fread(speedBuff, 1, sizeof speedBuff, fmaxSpeed);
    fclose(fmaxSpeed);
    ESP_LOGI(TAG, "%s", speedBuff);
}

static void vSaveMaxSpeed() {
    ESP_LOGI(TAG, "[SAVING] maxSpeed: %f", rideParams.maxSpeed);
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

// start with one value updated everytime > oldMaxSpeed
// append to file to save previous data? goto EOF and move poiner -sizeOf speed float val?
// use freeRTOS buffer/queue with timeout (number of reads? few seconds?) to debounce multiple values writes at start
// control file size?
// writing everysecond could wear flash after 1 year. buffer some data and write with longer intervals? (see freeRTOS buffer above)

void vSpiffsSyncOnStopTask(void* data) {
    // unblock on notification from rideStatus task
    // save max speed and others params to files
    vSaveMaxSpeed();
}