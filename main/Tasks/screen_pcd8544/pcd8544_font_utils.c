#include <pcd8544_font_utils.h>

#include "esp_log.h"

uint8_t **getSpeedChars(float *speed) {
    // lifetime array of pointers to chars
    static uint8_t *char_arr[4];
    // create buffer for converted float
    char buffer[10];
    // convert float to int dot int
    uint8_t speedInt = *speed;
    float tempFrac = *speed - speedInt;
    uint8_t fraction = (tempFrac * 10);
    // pad str with 0 if speed lower than 10
    if(speedInt < 10) {
        snprintf(buffer, 10, "0%d.%d", speedInt, fraction);  
    } else {
        snprintf(buffer, 10, "%d.%d", speedInt, fraction);
    }

    for (int i = 0; i <= 4; i++) {
        // convert to int and fill char_arr with pointers to big font characters
        if (buffer[i] != '.') {
            char single_char_buf[1] = {buffer[i]};
            char_arr[i] = fontDetermination[atoi(single_char_buf)];
        } else {
            // TODO add dot? drawing rectangle is buggy ofc
            continue;
        }
    }
    return &char_arr;
}