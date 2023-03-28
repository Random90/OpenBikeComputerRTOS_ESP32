#include <stdint.h>
/* ESP */
#include "esp_event_base.h"
#include "esp_log.h"
#include "esp_system.h"
/* BLE */
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "services/gap/ble_svc_gap.h"
/* IMPL */
#include "ble_main.h"
#include "gat_server.h"
/* OBC */
#include "obc.h"

#define TAG "BLE_MAIN"

static bool notifyState;
static uint16_t bleConnectionHandle;
uint8_t bleAddrType;
TimerHandle_t dataTimerHandle;

void vInitBle();

static void advertisingStart();
static int gapEventHandler(struct ble_gap_event *event, void *arg);
static void txDataTimerHandler();
static void hostTask(void *param);
static void bleOnSync(void);
static void bleOnReset(int reason);
static void txTimerReset();
static void txTimerStop();
static void printAddr(const void *addr);

void vInitBle() {
    esp_err_t returnCode = esp_nimble_init();

    if (returnCode != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init nimble %d \n", returnCode);
        return;
    }

    ble_hs_cfg.sync_cb = bleOnSync;
    ble_hs_cfg.reset_cb = bleOnReset;

    /* name, period/time,  auto reload, timer ID, callback */
    // this will sent data? @TODO task istead of timer?
    dataTimerHandle = xTimerCreate("bleDataTimer", pdMS_TO_TICKS(TX_TIMER_PERIOD_MS), pdTRUE, (void *)0, txDataTimerHandler);

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

/*
 * Enables advertising with parameters:
 *     o General discoverable mode
 *     o Undirected connectable mode
 */
static void advertisingStart(void) {
    struct ble_gap_adv_params adv_params;
    struct ble_hs_adv_fields fields;
    int returnCode;

    /*
     *  Set the advertisement data included in our advertisements:
     *     o Flags (indicates advertisement type and other general info)
     *     o Advertising tx power
     *     o Device name
     */
    memset(&fields, 0, sizeof(fields));

    /*
     * Advertise two flags:
     *      o Discoverability in forthcoming advertisement (general)
     *      o BLE-only (BR/EDR unsupported)
     */
    fields.flags = BLE_HS_ADV_F_DISC_GEN |
                   BLE_HS_ADV_F_BREDR_UNSUP;

    /*
     * Indicate that the TX power level field should be included; have the
     * stack fill this value automatically.  This is done by assigning the
     * special value BLE_HS_ADV_TX_PWR_LVL_AUTO.
     */
    fields.tx_pwr_lvl_is_present = 1;
    fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;

    fields.name = (uint8_t *)device_name;
    fields.name_len = strlen(device_name);
    fields.name_is_complete = 1;

    returnCode = ble_gap_adv_set_fields(&fields);
    if (returnCode != 0) {
        ESP_LOGE(TAG, "error setting advertisement data; rc=%d\n", returnCode);
        return;
    }

    /* Begin advertising */
    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    returnCode = ble_gap_adv_start(bleAddrType, NULL, BLE_HS_FOREVER,
                                   &adv_params, gapEventHandler, NULL);
    if (returnCode != 0) {
        ESP_LOGE(TAG, "error enabling advertisement; rc=%d\n", returnCode);
        return;
    }
}

static int gapEventHandler(struct ble_gap_event *event, void *arg) {
    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            /* A new connection was established or a connection attempt failed */
            ESP_LOGI(TAG, "connection %s; status=%d\n",
                     event->connect.status == 0 ? "established" : "failed",
                     event->connect.status);

            if (event->connect.status != 0) {
                /* Connection failed; resume advertising */
                advertisingStart();
            }
            bleConnectionHandle = event->connect.conn_handle;
            break;

        case BLE_GAP_EVENT_DISCONNECT:
            ESP_LOGI(TAG, "disconnect; reason=%d\n", event->disconnect.reason);
            /* Connection terminated; resume advertising */
            advertisingStart();
            break;

        case BLE_GAP_EVENT_ADV_COMPLETE:
            ESP_LOGI(TAG, "adv complete\n");
            advertisingStart();
            break;

        case BLE_GAP_EVENT_SUBSCRIBE:
            ESP_LOGI(TAG,
                     "subscribe event; cur_notify=%d\n value handle; val_handle=%d\n",
                     event->subscribe.cur_notify, cyclingServiceHandle);
            if (event->subscribe.attr_handle == cyclingServiceHandle) {
                notifyState = event->subscribe.cur_notify;
                txTimerReset();
            } else if (event->subscribe.attr_handle != cyclingServiceHandle) {
                notifyState = event->subscribe.cur_notify;
                txTimerStop();
            }
            ESP_LOGI(TAG, "BLE_GAP_SUBSCRIBE_EVENT: handle from subscribe=%d", bleConnectionHandle);
            break;

        case BLE_GAP_EVENT_MTU:
            MODLOG_DFLT(INFO, "mtu update event; conn_handle=%d mtu=%d\n",
                        event->mtu.conn_handle,
                        event->mtu.value);
            break;
    }

    return 0;
}

static void txDataTimerHandler() {
    static uint8_t data[2];
    int returnCode;
    struct os_mbuf *memoryBuf;

    if (!notifyState) {
        return;
    }

    data[0] = 0x06; /* contact of a sensor? */
    data[1] = rideParams.speed;

    memoryBuf = ble_hs_mbuf_from_flat(data, sizeof(data));
    /*
        Sends a “free-form” characteristic notification.
        This function consumes the supplied mbuf regardless of the outcome.
    */
    returnCode = ble_gattc_notify_custom(bleConnectionHandle, cyclingServiceHandle, memoryBuf);

    if (returnCode != 0) {
        ESP_LOGE(TAG, "Error notifying hrm characteristic; rc=%d", returnCode);
    }

    txTimerReset();
}

static void hostTask(void *param) {
    ESP_LOGI(TAG, "BLE Host Task Started");
    /* This function will return only when nimble_port_stop() is executed */
    nimble_port_run();
    esp_nimble_deinit();
}

static void bleOnSync(void) {
    uint8_t addr[6] = {0};
    int returnCode;

    returnCode = ble_hs_id_infer_auto(0, &bleAddrType);
    if (returnCode != 0) {
        ESP_LOGE(TAG, "error determining address type; use command to set explicitly");
        return;
    }

    returnCode = ble_hs_id_copy_addr(bleAddrType, addr, NULL);
    if (returnCode != 0) {
        ESP_LOGE(TAG, "error fetching address");
        return;
    }
    printAddr(addr);
    advertisingStart();
}

static void bleOnReset(int reason) {
    ESP_LOGE(TAG, "Resetting state; reason=%d\n", reason);
}

static void txTimerReset() {
    bool returnSuccess = xTimerReset(dataTimerHandle, 1000 / portTICK_PERIOD_MS) == pdPASS;

    if (!returnSuccess) {
        ESP_LOGE(TAG, "Error resetting timer");
    }
}

static void txTimerStop() {
    xTimerStop(dataTimerHandle, TX_TIMER_PERIOD_MS / portTICK_PERIOD_MS);
}

static void printAddr(const void *addr) {
    const uint8_t *u8p;
    u8p = addr;

    ESP_LOGI(TAG, "Device Address: %02x:%02x:%02x:%02x:%02x:%02x", u8p[5], u8p[4], u8p[3], u8p[2], u8p[1], u8p[0]);
}