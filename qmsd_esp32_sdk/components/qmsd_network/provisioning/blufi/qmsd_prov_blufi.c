#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"

#ifdef CONFIG_BT_BLUEDROID_ENABLED
#error "Not support bt-bluedroid now, pls try use nimble"
#endif

#include "esp_nimble_hci.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "console/console.h"
#include "qmsd_blufi_pri.h"
#include "esp_blufi_api.h"
#include "qmsd_utils.h"

#include "esp_blufi.h"
#include "qmsd_wifi.h"
#include "qmsd_wifi_sta.h"

#define TAG "blufi"

static bool ble_is_connected = false;
static bool blufi_init = false;
static wifi_config_t sta_config;

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_id == WIFI_EVENT_SCAN_DONE) {
        uint16_t apCount = 0;
        esp_wifi_scan_get_ap_num(&apCount);
        if (apCount == 0) {
            BLUFI_INFO("Nothing AP found");
            return;
        }
        wifi_ap_record_t *ap_list = (wifi_ap_record_t *)malloc(sizeof(wifi_ap_record_t) * apCount);
        if (!ap_list) {
            BLUFI_ERROR("malloc error, ap_list is NULL");
            return;
        }
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&apCount, ap_list));
        esp_blufi_ap_record_t * blufi_ap_list = (esp_blufi_ap_record_t *)malloc(apCount * sizeof(esp_blufi_ap_record_t));
        if (!blufi_ap_list) {
            if (ap_list) {
                free(ap_list);
            }
            BLUFI_ERROR("malloc error, blufi_ap_list is NULL");
            return;
        }
        for (int i = 0; i < apCount; ++i) {
            blufi_ap_list[i].rssi = ap_list[i].rssi;
            memcpy(blufi_ap_list[i].ssid, ap_list[i].ssid, sizeof(ap_list[i].ssid));
        }

        if (ble_is_connected == true) {
            esp_blufi_send_wifi_list(apCount, blufi_ap_list);
        } else {
            BLUFI_INFO("BLUFI BLE is not connected yet\n");
        }
        esp_wifi_scan_stop();
        free(ap_list);
        free(blufi_ap_list);
    } else if (event_id == QMSD_WIFI_EVENT_CONNECT) {
        esp_blufi_send_wifi_conn_report(WIFI_MODE_STA, ESP_BLUFI_STA_CONN_SUCCESS, 0, NULL);
    } else if (event_id == QMSD_WIFI_EVENT_DISCONNECT) {
        esp_blufi_send_wifi_conn_report(WIFI_MODE_STA, ESP_BLUFI_STA_CONN_FAIL, 0, NULL);
    }
  
    return;
}

static void blufi_event_callback(esp_blufi_cb_event_t event, esp_blufi_cb_param_t *param) {
    switch (event) {
        case ESP_BLUFI_EVENT_INIT_FINISH:
            BLUFI_INFO("BLUFI init finish");
            esp_blufi_adv_start();
            break;
        case ESP_BLUFI_EVENT_DEINIT_FINISH:
            BLUFI_INFO("BLUFI deinit finish");
            break;
        case ESP_BLUFI_EVENT_BLE_CONNECT:
            BLUFI_INFO("BLUFI ble connect");
            ble_is_connected = true;
            esp_blufi_adv_stop();
            blufi_security_init();
            break;
        case ESP_BLUFI_EVENT_BLE_DISCONNECT:
            BLUFI_INFO("BLUFI ble disconnect");
            ble_is_connected = false;
            blufi_security_deinit();
            if (blufi_init) {
                esp_blufi_adv_start();
            }
            break;
        case ESP_BLUFI_EVENT_SET_WIFI_OPMODE:
            if (param->wifi_mode.op_mode != WIFI_MODE_STA) {
                BLUFI_ERROR("Only support sta mode");
                break ;
            }
            ESP_ERROR_CHECK(esp_wifi_set_mode(param->wifi_mode.op_mode));
            break;
        case ESP_BLUFI_EVENT_REQ_CONNECT_TO_AP:
            BLUFI_INFO("BLUFI requset wifi connect to AP");
            qmsd_wifi_sta_disconnect();
            qmsd_wifi_sta_connect(&sta_config, WIFI_BW_HT20, 0);
            break;
        case ESP_BLUFI_EVENT_REQ_DISCONNECT_FROM_AP:
            BLUFI_INFO("BLUFI requset wifi disconnect from AP");
            qmsd_wifi_sta_disconnect();
            break;
        case ESP_BLUFI_EVENT_REPORT_ERROR:
            BLUFI_ERROR("BLUFI report error, error code %d", param->report_error.state);
            esp_blufi_send_error_info(param->report_error.state);
            break;
        case ESP_BLUFI_EVENT_GET_WIFI_STATUS: {
            wifi_mode_t mode;
            esp_blufi_extra_info_t info;
            esp_wifi_get_mode(&mode);
            if (qmsd_wifi_sta_get_status() == STA_CONNECTED) {
                memset(&info, 0, sizeof(esp_blufi_extra_info_t));
                wifi_config_t config;
                qmsd_wifi_sta_get_config(&config);
                info.sta_ssid = config.sta.ssid;
                info.sta_ssid_len = strlen((char*)info.sta_ssid);
                esp_blufi_send_wifi_conn_report(mode, ESP_BLUFI_STA_CONN_SUCCESS, 0, &info);
            } else {
                esp_blufi_send_wifi_conn_report(mode, ESP_BLUFI_STA_CONN_FAIL, 0, NULL);
            }
            BLUFI_INFO("BLUFI get wifi status from AP");
            break;
        }
        case ESP_BLUFI_EVENT_RECV_SLAVE_DISCONNECT_BLE:
            BLUFI_INFO("blufi close a gatt connection");
            esp_blufi_disconnect();
            break;
        case ESP_BLUFI_EVENT_DEAUTHENTICATE_STA:
            /* TODO */
            break;
        case ESP_BLUFI_EVENT_RECV_STA_BSSID:
            memcpy(sta_config.sta.bssid, param->sta_bssid.bssid, 6);
            sta_config.sta.bssid_set = 1;
            esp_wifi_set_config(WIFI_IF_STA, &sta_config);
            BLUFI_INFO("Recv STA BSSID %s", sta_config.sta.ssid);
            break;
        case ESP_BLUFI_EVENT_RECV_STA_SSID:
            strncpy((char *)sta_config.sta.ssid, (char *)param->sta_ssid.ssid, param->sta_ssid.ssid_len);
            sta_config.sta.ssid[param->sta_ssid.ssid_len] = '\0';
            esp_wifi_set_config(WIFI_IF_STA, &sta_config);
            BLUFI_INFO("Recv STA SSID %s", sta_config.sta.ssid);
            break;
        case ESP_BLUFI_EVENT_RECV_STA_PASSWD:
            strncpy((char *)sta_config.sta.password, (char *)param->sta_passwd.passwd, param->sta_passwd.passwd_len);
            sta_config.sta.password[param->sta_passwd.passwd_len] = '\0';
            esp_wifi_set_config(WIFI_IF_STA, &sta_config);
            BLUFI_INFO("Recv STA PASSWORD %s", sta_config.sta.password);
            break;
        case ESP_BLUFI_EVENT_RECV_SOFTAP_SSID:
        case ESP_BLUFI_EVENT_RECV_SOFTAP_PASSWD:
        case ESP_BLUFI_EVENT_RECV_SOFTAP_MAX_CONN_NUM:
        case ESP_BLUFI_EVENT_RECV_SOFTAP_AUTH_MODE:
        case ESP_BLUFI_EVENT_RECV_SOFTAP_CHANNEL:
            BLUFI_ERROR("Not support config softap");
            break;
        case ESP_BLUFI_EVENT_GET_WIFI_LIST:{
            BLUFI_INFO("Recv wifi list scan %s", sta_config.sta.password);
            wifi_scan_config_t scanConf = {
                .ssid = NULL,
                .bssid = NULL,
                .channel = 0,
                .show_hidden = false
            };
            esp_wifi_scan_start(&scanConf, false);
            break;
        }
        case ESP_BLUFI_EVENT_RECV_CUSTOM_DATA:
            BLUFI_INFO("Recv Custom Data %ld", param->custom_data.data_len);
            esp_log_buffer_hex("Custom Data", param->custom_data.data, param->custom_data.data_len);
            break;
        case ESP_BLUFI_EVENT_RECV_USERNAME:
        case ESP_BLUFI_EVENT_RECV_CA_CERT:
        case ESP_BLUFI_EVENT_RECV_CLIENT_CERT:
        case ESP_BLUFI_EVENT_RECV_SERVER_CERT:
        case ESP_BLUFI_EVENT_RECV_CLIENT_PRIV_KEY:
        case ESP_BLUFI_EVENT_RECV_SERVER_PRIV_KEY:
            /* Not handle currently */
            break;
        default:
            break;
    }
}

static esp_blufi_callbacks_t blufi_callbacks = { 
    .event_cb = blufi_event_callback,
    .negotiate_data_handler = blufi_dh_negotiate_data_handler,
    .encrypt_func = blufi_aes_encrypt,
    .decrypt_func = blufi_aes_decrypt,
    .checksum_func = blufi_crc_checksum,
};

esp_err_t qmsd_prov_blufi_start(const char* ble_adv_name) {
    if (blufi_init) {
        return ESP_FAIL;
    }
    blufi_init = true;
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_SCAN_DONE, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(QMSD_WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    esp_err_t ret;
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        BLUFI_ERROR("%s initialize bt controller failed: %s", __func__, esp_err_to_name(ret));
        return ret;
    }
    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        BLUFI_ERROR("%s enable bt controller failed: %s", __func__, esp_err_to_name(ret));
        return ret;
    }
    ret = esp_blufi_host_and_cb_init(ble_adv_name, &blufi_callbacks);
    if (ret) {
        BLUFI_ERROR("%s initialise failed: %s", __func__, esp_err_to_name(ret));
        return ret;
    }
    BLUFI_INFO("BLUFI VERSION %04x", esp_blufi_get_version());
    return ret;
}

esp_err_t qmsd_prov_blufi_get_result(wifi_config_t* wifi_config, uint32_t timeout) {
    if (qmsd_wifi_sta_wait_connect(timeout) != STA_CONNECTED) {
        return ESP_FAIL;
    }
    qmsd_wifi_sta_get_config(wifi_config);
    return ESP_OK;
}

esp_err_t qmsd_prov_blufi_deinit() {
    if (blufi_init == false) {
        return ESP_FAIL;
    }
    esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_SCAN_DONE, &wifi_event_handler);
    esp_event_handler_unregister(QMSD_WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler);
    ble_gap_adv_stop();
    esp_blufi_deinit();
    // Wait msg to device
    vTaskDelay(pdMS_TO_TICKS(500));
    blufi_init = false;
    blufi_security_deinit();
    esp_blufi_host_deinit();
    return ESP_OK;
}

bool qmsd_blufi_get_bleconnected() {
    return ble_is_connected;
}
