#include <stdio.h>

#include "aiha_websocket.h"
#include "audio_player_user.h"
#include "chat_notify.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "qmsd_prov_wifi.h"
#include "qmsd_utils.h"
#include "qmsd_wifi.h"
#include "qmsd_wifi_sta.h"
#include "storage_nvs.h"
#include "qmsd_ota.h"
#include "qmsd_network.h"

wifi_config_t* wifi_cfg;

#define TAG "qmsd.net"

static void wifi_cfg_via_blufi_task(void* arg) {
    chat_notify_audio_play(NOTIFY_NOT_BIND, NULL);
    qmsd_wifi_sta_set_reconnect_times(4, 4, 4);

    static char* device_name;
    if (device_name == NULL) {
        asprintf(&(device_name), "QM-%s", qmsd_get_device_mac_id());
    }
    qmsd_prov_blufi_start(device_name);
    wifi_config_t wifi_config;
    while (qmsd_prov_blufi_get_result(&wifi_config, portMAX_DELAY) != ESP_OK) {
        if (qmsd_wifi_sta_wait_connect(0) == STA_CONNECT_FAILED) {
            switch (qmsd_wifi_sta_get_failed_reason()) {
                case WIFI_REASON_AUTH_EXPIRE:
                case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:
                case WIFI_REASON_AUTH_FAIL:
                case WIFI_REASON_ASSOC_EXPIRE:
                case WIFI_REASON_HANDSHAKE_TIMEOUT:
                case WIFI_REASON_MIC_FAILURE:
                    chat_notify_audio_play(NOTIFY_WIFI_STA_AUTH_FAILED, NULL);
                    break;
                case WIFI_REASON_NO_AP_FOUND:
                    chat_notify_audio_play(NOTIFY_WIFI_AP_NOT_FOUND, NULL);
                    break;
                default:
                    chat_notify_audio_play(NOTIFY_WIFI_FAILED, NULL);
                    break;
            }
            qmsd_wifi_sta_clear_connect_status();
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    qmsd_prov_blufi_deinit();

    storage_nvs_write_blob("wifiCfg", &wifi_config, sizeof(wifi_config_t));
    chat_notify_audio_play(NOTIFY_WIFI_CONNECT, NULL);
    vTaskDelay(pdMS_TO_TICKS(6000));
    esp_restart();
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

bool qmsd_network_get_need_bind() {
    uint32_t len = sizeof(wifi_config_t);
    if (storage_nvs_read_blob("wifiCfg", (void**)&wifi_cfg, &len) != ESP_OK) {
        return true;
    }
    return false;
}

bool qmsd_network_get_connected() {
    if (qmsd_wifi_sta_get_status() != STA_CONNECTED) {
        return false;
    }
    return true;
}

void qmsd_network_task(void* arg) {
    uint8_t g_wifi_need_cfg = 0;
    qmsd_network_connect_success_cb_t success_cb = (qmsd_network_connect_success_cb_t)arg;
    // 初始化wifi
    qmsd_wifi_init();
    qmsd_wifi_sta_init();

    g_wifi_need_cfg = qmsd_network_get_need_bind();
    if (g_wifi_need_cfg) {
        audio_player_wait_idle(10000);
        wifi_cfg_via_blufi_task(NULL);
    }
    
    qmsd_wifi_sta_set_reconnect_times(-1, -1, -1);
    uint32_t connect_start_ticks = xTaskGetTickCount();
    qmsd_wifi_sta_connect(wifi_cfg, WIFI_BW_HT20, 0);

    while (qmsd_wifi_sta_get_status() != STA_CONNECTED) {
        if (connect_start_ticks && xTaskGetTickCount() - connect_start_ticks > pdMS_TO_TICKS(10000)) {
            chat_notify_audio_play(NOTIFY_WIFI_SERVER_ERROR, NULL);
            connect_start_ticks = 0;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    audio_player_wait_idle(10000);
    chat_notify_audio_play(NOTIFY_CONNECT_SUCCESS, NULL);
    qmsd_check_ota_by_http();
    if (success_cb) {
        success_cb();
    }

    vTaskDelete(NULL);
}

void qmsd_network_start(qmsd_network_connect_success_cb_t cb) {
    qmsd_thread_create(qmsd_network_task, "qmsd_network_task", 4 * 1024, cb, 5, NULL, 0, 1);
}
