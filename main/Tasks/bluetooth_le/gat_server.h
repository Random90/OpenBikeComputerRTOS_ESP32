#ifndef TASKS_BLUETOOTH_LE_GATT_SERVER_H_
#define TASKS_BLUETOOTH_LE_GATT_SERVER_H_

#include <stdio.h>

#define GATT_CSS_UUID 0x1816
#define GATT_CSS_MEASUREMENT_INTERVAL_UUID 0x2A21
#define GATT_CSS_CYCLING_APPEARANCE_UUID 0x2A01
#define GATT_CSS_CYCLING_COMP_APPEAR_UUID 0x0481

#define GATT_DEVICE_INFO_UUID 0x180A
#define GATT_DI_DEVICE_NAME_UUID 0x2A00

extern uint16_t cyclingServiceHandle;
static const char *device_name = "Microb OpenBikeComputer";

int gatServerInit(void);

#endif /* TASKS_BLUETOOTH_LE_GATT_SERVER_H_ */