#pragma once

#include "esp_wifi.h"

typedef enum {
    STA_NOT_CONNECTED,
    STA_CONNECTED,
    STA_RECONNECT,
    STA_CONNECTING,
    STA_CONNECT_FAILED
} qmsd_wifi_status_t;

#ifdef __cplusplus
extern "C" {
#endif

void qmsd_wifi_sta_init();

qmsd_wifi_status_t qmsd_wifi_sta_connect(wifi_config_t* conf, wifi_bandwidth_t bandwidth, uint32_t timeout_ms);

void qmsd_wifi_sta_disconnect();

qmsd_wifi_status_t qmsd_wifi_sta_get_status();

qmsd_wifi_status_t qmsd_wifi_sta_wait_connect(uint32_t timeout_ms);

qmsd_wifi_status_t qmsd_wifi_sta_wait_connect_success(uint32_t timeout);

int8_t qmsd_wifi_sta_get_rssi();

const char *qmsd_wifi_sta_get_ip();

void qmsd_wifi_sta_clear_connect_status();

esp_err_t qmsd_wifi_sta_reconnect();

wifi_err_reason_t qmsd_wifi_sta_get_failed_reason();

void qmsd_wifi_sta_set_reconnect_times(int32_t auth, int32_t ap_not_found, int32_t other);

qmsd_wifi_status_t qmsd_wifi_sta_get_config(wifi_config_t* config);

#ifdef __cplusplus
}
#endif
