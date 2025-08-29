#include <stdio.h>
#include "esp_err.h"
#include "esp_blufi_api.h"
#include "esp_log.h"
#include "esp_blufi.h"
#include "esp_nimble_hci.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "console/console.h"
#include "qmsd_blufi_pri.h"

void ble_store_config_init(void);
static void blufi_on_reset(int reason) {
    MODLOG_DFLT(ERROR, "Resetting state; reason=%d", reason);
}

static void blufi_on_sync(void) {
  esp_blufi_profile_init();
}

void bleprph_host_task(void *param) {
    BLUFI_INFO("BLE Host Task Started");
    /* This function will return only when nimble_port_stop() is executed */
    nimble_port_run();
    nimble_port_freertos_deinit();
}

esp_err_t esp_blufi_host_init(const char* ble_adv_name) {
    esp_log_level_set("BLUFI_EXAMPLE", ESP_LOG_WARN);
    // ESP_ERROR_CHECK(esp_nimble_hci_init());
    // nimble_port_init();
    esp_err_t err;
    err = esp_nimble_init();
    if (err) {
        BLUFI_ERROR("%s failed: %s\n", __func__, esp_err_to_name(err));
        return ESP_FAIL;
    }

    /* Initialize the NimBLE host configuration. */
    ble_hs_cfg.reset_cb = blufi_on_reset;
    ble_hs_cfg.sync_cb = blufi_on_sync;
    ble_hs_cfg.gatts_register_cb = esp_blufi_gatt_svr_register_cb;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;
    ble_hs_cfg.sm_io_cap = 4;
    ble_hs_cfg.sm_sc = 0;

    int rc;
    rc = esp_blufi_gatt_svr_init();
    assert(rc == 0);
    /* Set the default device name. */
    BLUFI_INFO("Adv name: %s", ble_adv_name);
    rc = ble_svc_gap_device_name_set(ble_adv_name);
    assert(rc == 0);
    ble_store_config_init();
    esp_blufi_btc_init();
    nimble_port_freertos_init(bleprph_host_task);
    return ESP_OK;
}

esp_err_t esp_blufi_gap_register_callback(void) {
    return ESP_OK;
}

esp_err_t esp_blufi_host_and_cb_init(const char* ble_adv_name, esp_blufi_callbacks_t *callbacks) {
    esp_err_t ret = ESP_OK;

    ret = esp_blufi_register_callbacks(callbacks);
    if(ret){
        BLUFI_ERROR("%s blufi register failed, error code = %x", __func__, ret);
        return ret;
    }

    ret = esp_blufi_gap_register_callback();
    if(ret){
        BLUFI_ERROR("%s gap register failed, error code = %x", __func__, ret);
        return ret;
    }

    ret = esp_blufi_host_init(ble_adv_name);
    if (ret) {
        BLUFI_ERROR("%s initialise host failed: %s", __func__, esp_err_to_name(ret));
        return ret;
    }

    return ret;
}

esp_err_t esp_blufi_host_deinit() {
    int ret = nimble_port_stop();
    if (ret == 0) {
        nimble_port_deinit();
        // ret = esp_nimble_hci_and_controller_deinit();
        // if (ret != ESP_OK) {
        //     ESP_LOGE("Blufi", "esp_nimble_hci_and_controller_deinit() failed with error: %d", ret);
        // }
        extern void btc_deinit(void);
        btc_deinit();
    } else {
        ESP_LOGE("Blufi", "stop error");
    }
    // Wait btc task exit
    vTaskDelay(pdMS_TO_TICKS(100));
    return ESP_OK;
}
