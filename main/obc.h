#ifndef OBC_CORE
#define OBC_CORE

#include "freertos/FreeRTOS.h"
#include "driver/spi_common.h"
#include "pcd8544.h"
// OBC specific global structs
 typedef struct {
    bool moving;
    uint32_t rotations;
    portTickType prevRotationTickCount;
    portTickType rotationTickCount;
    uint16_t msBetweenRotationTicks;
    float speed;
    float distance;
 } ride_params_t;

#endif
