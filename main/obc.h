#ifndef OBC_CORE
#define OBC_CORE


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
 } ride_params_t;

#endif
