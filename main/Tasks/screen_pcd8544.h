#ifndef SCREEN_PCD8544
#define SCREEN_PCD8544
/* ^^ these are the include guards */
#include "../obc.h"
#include <u8g2.h>

#define REFRESH_RATE_MS 500

extern ride_params_t rideParams;
extern u8g2_t u8g2;

/* Task used to refresh the pcd8544 screen displayed information every half second
*/

void vScreenRefreshTask(void* data);

#endif