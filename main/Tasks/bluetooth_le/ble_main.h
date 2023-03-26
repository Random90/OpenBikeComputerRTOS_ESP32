#ifndef TASKS_BLUETOOTH_LE_BLE_MAIN_H_
#define TASKS_BLUETOOTH_LE_BLE_MAIN_H_

#include "nimble/ble.h"

#define TX_TIMER_PERIOD_MS 1000

struct ble_hs_cfg;

void vInitBle(void);

#endif /* TASKS_BLUETOOTH_LE_BLE_MAIN_H_ */