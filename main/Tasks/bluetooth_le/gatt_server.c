#ifdef NO_SCREEN

#include "gatt_server.h"

#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

uint16_t cyclingServiceHandle;
// static int gattServerAccessMeasurmentInterval(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);
static int gattServerAccessAppearance(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);
static int gattServerAccessDeviceName(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);

static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {/* Service: Cycling Speed and Cadence service */
     .type = BLE_GATT_SVC_TYPE_PRIMARY,
     .uuid = BLE_UUID16_DECLARE(GATT_CSS_UUID),
     .characteristics = (struct ble_gatt_chr_def[]){
         {
             /* Characteristic: Appearance */
             .uuid = BLE_UUID16_DECLARE(GATT_CSS_CYCLING_APPEARANCE_UUID),
             .access_cb = gattServerAccessAppearance,
             .flags = BLE_GATT_CHR_F_READ,
         },
         {
             /* Characteristic: Measurement interval */
             .uuid = BLE_UUID16_DECLARE(GATT_CSS_MEASUREMENT_INTERVAL_UUID),
             //  .access_cb = gattServerAccessMeasurmentInterval,
             .val_handle = &cyclingServiceHandle,
             .flags = BLE_GATT_CHR_F_NOTIFY,
         },
         {
             0, /* No more characteristics in this service */
         },
     }},

    {/* Service: Device Information */
     .type = BLE_GATT_SVC_TYPE_PRIMARY,
     .uuid = BLE_UUID16_DECLARE(GATT_DEVICE_INFO_UUID),
     .characteristics = (struct ble_gatt_chr_def[]){
         {
             /* Characteristic: * Device name */
             .uuid = BLE_UUID16_DECLARE(GATT_DI_DEVICE_NAME_UUID),
             .access_cb = gattServerAccessDeviceName,
             .flags = BLE_GATT_CHR_F_READ,
         },
         {
             0, /* No more characteristics in this service */
         },
     }},

    {
        0, /* No more services */
    },
};

int gatServerInit(void) {
    int returnCode;

    ble_svc_gap_init();
    ble_svc_gatt_init();

    returnCode = ble_gatts_count_cfg(gatt_svr_svcs);
    if (returnCode != 0) {
        return returnCode;
    }

    returnCode = ble_gatts_add_svcs(gatt_svr_svcs);
    if (returnCode != 0) {
        return returnCode;
    }

    return 0;
}

static int gattServerAccessAppearance(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg) {
    uint16_t uuid;
    uint16_t appearance = GATT_CSS_CYCLING_COMP_APPEAR_UUID;
    int returnCode;

    uuid = ble_uuid_u16(ctxt->chr->uuid);

    switch (uuid) {
        case GATT_CSS_CYCLING_APPEARANCE_UUID:
            returnCode = os_mbuf_append(ctxt->om, &appearance, sizeof(appearance));
            break;
        case GATT_DI_DEVICE_NAME_UUID:
            returnCode = os_mbuf_append(ctxt->om, device_name, sizeof(device_name));
            break;
        default:
            return BLE_ATT_ERR_UNSUPPORTED_GROUP;
    }

    return returnCode == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}

#endif