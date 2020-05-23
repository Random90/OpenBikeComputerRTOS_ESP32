#ifndef OBC_CORE
#define OBC_CORE

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

// OBC specific global structs
 typedef struct ride_params_t {
    bool moving;
    uint32_t rotations;
    portTickType prevRotationTickCount;
    portTickType rotationTickCount;
    uint16_t msBetweenRotationTicks;
    uint32_t totalRideTimeMs;
    float speed;
    float avgSpeed;
    float distance;
    float maxSpeed;
    float totalDistance;
 } ride_params_t;

extern ride_params_t rideParams;

//shared queues
extern xQueueHandle reed_evt_queue;

//shared tasks
extern TaskHandle_t screenRefreshTaskHandle;
extern TaskHandle_t spiffsSyncOnStopTaskHandle;

#endif
