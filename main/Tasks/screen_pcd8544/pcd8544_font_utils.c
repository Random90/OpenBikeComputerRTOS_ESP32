#include <pcd8544_font_utils.h>
#include "esp_log.h"

uint8_t **getSpeedChars(float *speed) {
    // lifetime array of pointers to chars
    static uint8_t *char_arr[4];
    // create buffer for converted float
    char buffer[4];
    // convert float to string
    int size = snprintf(buffer, sizeof buffer, "%0.2f", *speed);
    // pad str with 0 if speed lower than 10
    if (size < sizeof(buffer)) {
        buffer[0] = '0';
    }
    for (int i = 0; i<=sizeof buffer; i++) {
        // convert to int and fill char_arr with pointers to big font characters
        if (buffer[i] != '.') {
            char single_char_buf[1] = {buffer[i]};
            char_arr[i] = fontDetermination[atoi(single_char_buf)];
            ESP_LOGI("TEST","%d, %d %p", atoi(single_char_buf),i,char_arr[i]);
        } else {
            // TODO assign dot
            char_arr[i] = fontDetermination[0];
        }
        
        
    }
    return &char_arr;
}