#include <pcd8544_font_utils.h>

#include "esp_log.h"

/**
 * @param charArr array of chars to which font data will be passed
 * @param buffer source of text
 * @param nrOfChars number of characters to copy as font from buffer
 * @brief Translates string to array of binary font data.
 * @warning Skips dots (null pointer), dont includes special chars
 * @TODO validate string, skip special chars
 * */
static void fillCharsFromBuffer(uint8_t **charArr, char *buffer, int nrOfChars) {
    for (int i = 0; i <= nrOfChars; i++) {
        // convert to int and fill charArr with pointers to big font characters
        if (*buffer != '.') {
            char single_char_buf[1] = {*buffer};
            *charArr = fontDetermination[atoi(single_char_buf)];
        } else {
            // returns dot
            *charArr = fontDetermination[10];
        }
        buffer++;
        charArr++;
    }
}
/**
 * @brief convert bike speed from float to included font char array
 * @return pointer to array of binary characters
 * */
uint8_t **getSpeedChars(float *speed) {
    // lifetime array of pointers to chars
    static uint8_t *charArr[4];
    // create buffer for converted float
    char buffer[10];
    // prevent overflow
    if (*speed > 99.9) {
        *speed = 99.9;
    }

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
    fillCharsFromBuffer(charArr, buffer, 4);
    return &charArr;
}
/**
 * @brief convert distance measured from float to included font char array
 * @return pointer to array of binary characters
 * */
// uint8_t **getDistanceChars(float *distance) {
//     static uint8_t *charArr[4];
// }