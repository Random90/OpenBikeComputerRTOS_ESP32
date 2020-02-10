#ifndef SCREEN_PCD8544
#define SCREEN_PCD8544
/* ^^ these are the include guards */
#include "obc.h"


#define REFRESH_RATE_MS 500

extern ride_params_t rideParams;

/* Task used to refresh the pcd8544 screen displayed information every half second
*/

void vScreenRefreshTask(void* data);

#endif