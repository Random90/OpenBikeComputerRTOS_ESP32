#ifndef TASKS_SCREEN_PCD8544_PCD8544_FONT_UTILS_H
#define TASKS_SCREEN_PCD8544_PCD8544_FONT_UTILS_H

#include <pcd8544_font16x24.h>
#include "../../utils/macros.h"
#include <stdio.h>
#include <stdlib.h>

// get array containing two pointers to arrays of bytes representing an char from selected font
uint8_t **getSpeedChars(float *value);

#endif