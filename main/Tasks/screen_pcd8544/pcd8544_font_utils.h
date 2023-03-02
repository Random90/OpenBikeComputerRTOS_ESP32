#ifndef TASKS_SCREEN_PCD8544_PCD8544_FONT_UTILS_H
#define TASKS_SCREEN_PCD8544_PCD8544_FONT_UTILS_H

#include <pcd8544_font16x24.h>
#include <stdio.h>
#include <stdlib.h>

#include "../../utils/macros.h"

/**
 * @brief convert bike speed from float to included font char array
 * @param charRowsArr pointer to empty array, which will be filled with next chars row number
 * @return pointer to array of binary characters
 * */
void vGetSpeedChars(uint8_t *charArr[4], float *speed, uint8_t (*bigCharPositions)[6]);

/**
 * @brief convert distance measured from float to included font char array
 * @param charRowsArr pointer to empty array, which will be filled with next chars row number
 * @return pointer to array of binary characters
 * */
void vGetDistanceChars(uint8_t *charArr[6], float *distance, uint8_t (*bigCharPositions)[6]);

#endif