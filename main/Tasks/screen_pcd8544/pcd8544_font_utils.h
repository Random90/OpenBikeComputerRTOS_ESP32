#ifndef TASKS_SCREEN_PCD8544_PCD8544_FONT_UTILS_H
#define TASKS_SCREEN_PCD8544_PCD8544_FONT_UTILS_H

#include <pcd8544_font16x24.h>
#include "../../utils/macros.h"
#include <stdio.h>
#include <stdlib.h>

/**
 * @brief convert bike speed from float to included font char array
 * @return pointer to array of binary characters
 * */
uint8_t **getSpeedChars(float *value);

/**
 * @brief convert distance measured from float to included font char array
 * @return pointer to array of binary characters
 * */
uint8_t **getDistanceChars(float *distance);

#endif