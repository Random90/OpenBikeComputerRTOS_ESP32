#ifndef TASKS_SCREEN_PCD8544_PCD8544_FONT_UTILS_H
#define TASKS_SCREEN_PCD8544_PCD8544_FONT_UTILS_H

#include <pcd8544_font16x24.h>
#include "../../utils/macros.h"
#include <stdio.h>
#include <stdlib.h>

/**
 * @brief convert bike speed from float to included font char array
 * @param charRowsArr pointer to empty array, which will be filled with next chars row number
 * @return pointer to array of binary characters
 * */
uint8_t **getSpeedChars(float *value, uint8_t *charRowsArr);

/**
 * @brief convert distance measured from float to included font char array
 * @param charRowsArr pointer to empty array, which will be filled with next chars row number
 * @return pointer to array of binary characters
 * */
uint8_t **getDistanceChars(float *distance, uint8_t *charRowsArr);

#endif