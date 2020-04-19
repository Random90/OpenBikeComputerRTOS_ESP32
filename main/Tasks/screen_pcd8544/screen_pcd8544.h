#ifndef TASKS_SCREEN_PCD8544_SCREEN_PCD8544
#define TASKS_SCREEN_PCD8544_SCREEN_PCD8544
/* ^^ these are the include guards */
#include "../obc.h"

#define REFRESH_RATE_MS 500
#define POWER_SAVE_DELAY_MS 10000

#define SCREEN_0 (1 << 0) // 1
#define SCREEN_1 (1 << 1) // 2
#define SCREEN_2 (1 << 2) // 4
#define SCREEN_3 (1 << 3) // 8
#define SCREEN_4 (1 << 4) // 16
#define SCREEN_5 (1 << 5) // 32
#define SCREEN_6 (1 << 6) // 64
#define SCREEN_7 (1 << 7) // 128

extern ride_params_t rideParams;

// Task used to refresh the pcd8544 screen displayed information every half second
void vScreenRefreshTask(void* data);

#endif