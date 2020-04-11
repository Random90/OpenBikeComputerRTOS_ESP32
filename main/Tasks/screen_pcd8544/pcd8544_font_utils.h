#ifndef TASKS_SCREEN_PCD8544_PCD8544_FONT_UTILS_H
#define TASKS_SCREEN_PCD8544_PCD8544_FONT_UTILS_H

#include <pcd8544_font16x24.h>

struct two_digits_ptrs {
    uint8_t (*first)[CHAR_SIZE];
    uint8_t (*second)[CHAR_SIZE];
};

// get struct containing two pointers to arrays of bytes representing an char from selected font
struct two_digits_ptrs *getChars(int value);

#endif