#ifdef NO_SCREEN
#include "esp_event_base.h"
#include "esp_log.h"
#include "esp_system.h"

/* BLE */
#include "ble_main.h"
#include "gatt_server.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "services/gap/ble_svc_gap.h"

/* OBC */
#include "obc.h"

#define TAG "BLE_MAIN"

static bool notifyState;
static uint16_t bleConnectionHandle;

static void hostTask(void *param);
static void txDataTimerHandler(TimerHandle_t timerHandler);
static void bleOnSync(void);
static void bleOnReset(int reason);
static void txTimerReset(TimerHandle_t timerHandler);
static void printAddr(const void *addr);

void vInitBle() {
    esp_err_t returnCode = esp_nimble_init();
    TimerHandle_t bleTimer;

    if (returnCode != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init nimble %d \n", returnCode);
        return;
    }

    ble_hs_cfg.sync_cb = bleOnSync;
    ble_hs_cfg.reset_cb = bleOnReset;

    /* name, period/time,  auto reload, timer ID, callback */
    // this will sent data? @TODO task istead of timer?
    bleTimer = xTimerCreate("blehr_tx_timer", pdMS_TO_TICKS(TX_TIMER_PERIOD_MS), pdTRUE, (void *)0, txDataTimerHandler);

    returnCode = gatServerInit();
    if (returnCode != 0) {
        ESP_LOGE(TAG, "Failed to init gatt server %d \n", returnCode);
        return;
    }

    /* Set the default device name */
    returnCode = ble_svc_gap_device_name_set(device_name);
    if (returnCode != 0) {
        ESP_LOGE(TAG, "Error setting device name; rc=%d", returnCode);
        return;
    }
    nimble_port_freertos_init(hostTask);
}

static void txDataTimerHandler(TimerHandle_t timerHandler) {
    static uint8_t data[2];
    int returnCode;
    struct os_mbuf *memoryBuf;

    if (!notifyState) {
        xTimerStop(timerHandler, TX_TIMER_PERIOD_MS / portTICK_PERIOD_MS);
        return;
    }

    data[0] = 0x06; /* contact of a sensor? */
    data[1] = rideParams.speed;

    memoryBuf = ble_hs_mbuf_from_flat(data, sizeof(data));
    /*
        Sends a “free-form” characteristic notification.
        This function consumes the supplied mbuf regardless of the outcome.
    */
    returnCode = ble_gatts_notify_custom(bleConnectionHandle, cyclingServiceHandle, memoryBuf);

    if (returnCode != 0) {
        ESP_LOGE(TAG, "Error notifying hrm characteristic; rc=%d", returnCode);
    }

    txTimerReset(timerHandler);
}

void hostTask(void *param) {
    ESP_LOGI(TAG, "BLE Host Task Started");
    /* This function will return only when nimble_port_stop() is executed */
    nimble_port_run();
    esp_nimble_deinit();
}

static void bleOnSync(void) {
    uint8_t bleAddrType;
    uint8_t addr[6] = {0};
    int returnCode;

    int status = ble_hs_id_infer_auto(0, &bleAddrType);
    if (returnCode != 0) {
        ESP_LOGE(TAG, "error determining address type; use command to set explicitly");
        return;
    }

    returnCode = ble_hs_id_copy_addr(bleAddrType, addr, NULL);
    print_addr(addr);
    // @TODO implement
    blehr_advertise;
}

static void txTimerReset(TimerHandle_t timerHandler) {
    bool returnSuccess = xTimerReset(timerHandler, 1000 / portTICK_PERIOD_MS) == pdPASS;

    if (!returnSuccess) {
        ESP_LOGE(TAG, "Error resetting timer");
    }
}

static void bleOnReset(int reason) {
    ESP_LOGE(TAG, "Resetting state; reason=%d\n", reason);
}

static void printAddr(const void *addr) {
    const uint8_t *u8p;
    u8p = addr;

    ESP_LOGI(TAG, "Device Address: %02x:%02x:%02x:%02x:%02x:%02x", u8p[5], u8p[4], u8p[3], u8p[2], u8p[1], u8p[0]);
}

#endif