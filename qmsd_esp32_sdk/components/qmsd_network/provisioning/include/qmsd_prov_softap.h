#pragma once

#include <esp_wifi.h>

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t qmsd_prov_softap_start(const char* ssid, const char* password, const char* device_id);

esp_err_t qmsd_prov_ap_stop();

esp_err_t qmsd_prov_ap_get_result(wifi_config_t* wifi_config, uint32_t timeout);

#ifdef __cplusplus
}
#endif
