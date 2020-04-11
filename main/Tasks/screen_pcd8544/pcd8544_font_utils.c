#include <pcd8544_font_utils.h>

struct two_digits_ptrs *getChars(int value) {
    // lifetime array of pointers to chars
    static struct two_digits_ptrs *font_char;
    *font_char->first = &fontDetermination[0];
    *font_char->second = fontDetermination + 1;
    return font_char;
}