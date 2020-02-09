#ifndef SCREEN_PCD8544
#define SCREEN_PCD8544
/* ^^ these are the include guards */
#include "obc.h"


extern ride_params_t rideParams;

/* Task used to refresh the pcd8544 screen displayed information every half second
*/

void vScreenRefresh(void* data);

#endif