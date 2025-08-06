#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// Maybe used memory 57124 Byte
esp_err_t qmsd_prov_blufi_start(const char* ble_adv_name);

esp_err_t qmsd_prov_blufi_get_result(wifi_config_t* wifi_config, uint32_t timeout);

// If wifi connect, it`s will cause wifi disconnect
esp_err_t qmsd_prov_blufi_deinit();

// Get ble connected status
bool qmsd_blufi_get_bleconnected();

#ifdef __cplusplus
}
#endif
