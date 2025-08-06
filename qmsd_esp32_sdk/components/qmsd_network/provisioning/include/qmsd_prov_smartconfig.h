#pragma once

#include <esp_wifi.h>
#include <esp_smartconfig.h>

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t qmsd_prov_smartconfig_start(smartconfig_type_t type);

esp_err_t qmsd_prov_smartconfig_stop();

esp_err_t qmsd_prov_smartconfig_get_result(wifi_config_t* wifi_config, uint32_t timeout);

#ifdef __cplusplus
}
#endif
