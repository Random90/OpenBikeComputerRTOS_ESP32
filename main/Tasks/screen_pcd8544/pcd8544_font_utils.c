#include <pcd8544_font_utils.h>

#include "esp_log.h"
/**
 * @param charArr array of chars to which font data will be passed
 * @param buffer source of text
 * @param nrOfChars number of characters to copy as font from buffer
 * @param charRows pointer to array to which size of next chars will be passed
 * @brief Translates string to array of binary font data.
 * @warning Skips dots (null pointer), dont includes special chars
 * @TODO validate string, skip special chars, cleanup array sizes!
 * @TODO dynamic rows number calculation
 * */
static void fillCharsFromBuffer(uint8_t **charArr, char *buffer, int nrOfChars, uint8_t *charRowsArr) {
    for (int i = 0; i <= nrOfChars; i++) {
        // convert to int and fill charArr with pointers to big font characters
        if (*buffer != '.' && *buffer >= '0' && *buffer <= '9') {
            char single_char_buf[1] = {*buffer};
            *charArr = fontDetermination[atoi(single_char_buf)];
            charRowsArr[i] = 16;
        } else if(*buffer == '.'){
            *charArr = fontDetermination[10];
            charRowsArr[i] = 3;
        } else {
            *charArr = NULL;
            charRowsArr[i] = NULL;
        }
        buffer++;
        charArr++;
    }
}

uint8_t **getSpeedChars(float *speed, uint8_t *charRowsArr) {
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
    fillCharsFromBuffer(charArr, buffer, 4, charRowsArr);
    return &charArr;
}

uint8_t **getDistanceChars(float *distance, uint8_t *charRowsArr) {
    static uint8_t *charArr[6];
    char buffer[10];
    // @TODO handle distance > 999.9
    // convert float to int dot int
    uint8_t distanceInt = *distance;
    float tempFrac = *distance - distanceInt;
    uint8_t fraction = (tempFrac * 100);
    if(fraction < 10) {
        snprintf(buffer, 10, "%d.0%d", distanceInt, fraction);
    } else {
        snprintf(buffer, 10, "%d.%d", distanceInt, fraction);
    }
    fillCharsFromBuffer(charArr, buffer, 10, charRowsArr);
    return &charArr;
}