#ifndef TASKS_SCREEN_PCD8544_PCD8544_FONT_UTILS_H
#define TASKS_SCREEN_PCD8544_PCD8544_FONT_UTILS_H

#include <pcd8544_font16x24.h>

typedef struct aa {
    uint8_t *first, *second;
} two_digits_ptrs;

// get struct containing two pointers to arrays of bytes representing an char from selected font
two_digits_ptrs *getChars(int value);

#endif