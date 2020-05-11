#include "esp_spiffs.h"
#include "esp_log.h"
#include "esp_err.h"

#include "spiffs_main.h"


static const char *TAG = "SPIFFS_MAIN";

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


}

void vSpiffsMainTask(void* data) {

}