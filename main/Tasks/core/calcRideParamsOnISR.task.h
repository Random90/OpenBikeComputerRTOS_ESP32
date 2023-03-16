#ifndef TASKS_CORE_CALC_RIDE_PARAMS_ON_ISR_H
#define TASKS_CORE_CALC_RIDE_PARAMS_ON_ISR_H

// will ignore reed events for this time after last event to filter out bouncing/noise
#define REED_BOUNCE_MS 15
// calculates ride params on reed isr queue receive
void vCalcRideParamsOnISRTask(void* data);

#endif