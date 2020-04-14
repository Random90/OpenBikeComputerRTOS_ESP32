#include <pcd8544_font_utils.h>
#include "esp_log.h"

#define TAG "PCD8544"

uint8_t **getSpeedChars(float *speed) {
    // lifetime array of pointers to chars
    static uint8_t *char_arr[4];
    // create buffer for converted float
    char buffer1[5];
    char buffer2[6];
    // convert float to string
    int size = snprintf(buffer1, 4,"%0.2f", *speed);
    // pad str with 0 if speed lower than 10
    // FIXME optimize
    if (size < sizeof(buffer1)) {
        sprintf(buffer2, "0%s", buffer1);
    } else {
        sprintf(buffer2, "%s", buffer1);
    }
    ESP_LOGI(TAG, "%s", buffer1);
    for (int i = 0; i <= 4; i++) {
        // convert to int and fill char_arr with pointers to big font characters
        if (buffer2[i] != '.') {
            char single_char_buf[1] = {buffer2[i]};
            char_arr[i] = fontDetermination[atoi(single_char_buf)];
        } else {
            // TODO add dot? drewinf rectangle is buggy
            continue;
        }
        
        
    }
    return &char_arr;
}