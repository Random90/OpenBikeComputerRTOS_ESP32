#include <pcd8544_font_utils.h>
#include "esp_log.h"

two_digits_ptrs *getChars(int value) {
    // lifetime array of pointers to chars
    static two_digits_ptrs font_char;
    uint8_t *test_ptr = fontDetermination;
    ESP_LOGI("TEST","%p - %p -  %p", &fontDetermination[0], fontDetermination, fontDetermination[0]);
    ESP_LOGI("VALS", "%u %u", fontDetermination[0][2], test_ptr[2]);
    font_char.first = fontDetermination;
    font_char.second = fontDetermination + 1;
    ESP_LOGI("STRUCT", "%u", font_char.first[2]);
    return &font_char;
}