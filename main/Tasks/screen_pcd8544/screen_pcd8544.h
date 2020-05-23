#ifndef TASKS_SCREEN_PCD8544_SCREEN_PCD8544
#define TASKS_SCREEN_PCD8544_SCREEN_PCD8544
/* ^^ these are the include guards */
#include "../obc.h"

#define REFRESH_RATE_MS 500
#define POWER_SAVE_DELAY_MS 30000
#define SCREEN_CHANGE_AFTER_MS 5000

#define IMPLEMENTED_SCREENS 2

// Task used to refresh the pcd8544 screen displayed information every half second
void vScreenRefreshTask(void* data);

// init screen hardware
void vInitPcd8544Screen();
#endif