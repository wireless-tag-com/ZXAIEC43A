#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>

#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>

#include "qmsd_wifi_sta.h"
#include "qmsd_wifi.h"

#define TAG "qmsd_wifi_sta"

#define QMSD_WIFI_STA_NOT_CONNECT     BIT0
#define QMSD_WIFI_STA_CONNECTED       BIT1
#define QMSD_WIFI_STA_CONNECTING      BIT2
#define QMSD_WIFI_STA_CONNECT_FAILED  BIT3
#define QMSD_WIFI_STA_RECONNECTED     BIT4

static int32_t auth_num = 4;
static int32_t ap_not_found_num = 10;
static int32_t other_num = -1;
static wifi_config_t sta_config;
static wifi_err_reason_t disconnect_reason = 0;
static EventGroupHandle_t qmsd_wifi_event_group = NULL;
static char g_ip_addr[30];

static void qmsd_wifi_sta_set_status(qmsd_wifi_status_t status) {
    if (qmsd_wifi_event_group == NULL) {
        return ;
    }
    EventBits_t set_bits = 0x00;
    switch (status) {
        case STA_NOT_CONNECTED:
            set_bits = QMSD_WIFI_STA_NOT_CONNECT;
            break;
        case STA_CONNECTED:
            set_bits = QMSD_WIFI_STA_CONNECTED;
            break;
        case STA_CONNECTING:
            set_bits = QMSD_WIFI_STA_CONNECTING;
            break;
        case STA_CONNECT_FAILED:
            set_bits = QMSD_WIFI_STA_CONNECT_FAILED;
            break;
        case STA_RECONNECT:
            set_bits = QMSD_WIFI_STA_RECONNECTED;
            break;
    }

    xEventGroupClearBits(qmsd_wifi_event_group, 0xffffff);
    xEventGroupSetBits(qmsd_wifi_event_group, set_bits);
}

qmsd_wifi_status_t qmsd_wifi_sta_get_status() {
    if (qmsd_wifi_event_group == NULL) {
        return STA_NOT_CONNECTED;
    }

    EventBits_t bits = xEventGroupGetBits(qmsd_wifi_event_group);
    
    if (bits & QMSD_WIFI_STA_NOT_CONNECT) {
        return STA_NOT_CONNECTED;
    }

    if (bits & QMSD_WIFI_STA_CONNECTING) {
        return STA_CONNECTING;
    }

    if (bits & QMSD_WIFI_STA_CONNECTED) {
        return STA_CONNECTED;
    }

    if (bits & QMSD_WIFI_STA_CONNECT_FAILED) {
        return STA_CONNECT_FAILED;
    }

    if (bits & QMSD_WIFI_STA_RECONNECTED) {
        return STA_RECONNECT;
    }
    return STA_CONNECTING;
}

static void qmsd_wifi_connect_timer(TimerHandle_t xTimer) {
    esp_wifi_connect();
    xTimerDelete(xTimer, portMAX_DELAY);
}

static void qmsd_wifi_sta_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    static int32_t connect_num = 0;
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        // Onle sta start
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
        ESP_LOGI(TAG, "Connected with IP Address:" IPSTR, IP2STR(&event->ip_info.ip));
        sprintf(g_ip_addr, IPSTR, IP2STR(&event->ip_info.ip));
        esp_event_post(QMSD_WIFI_EVENT, QMSD_WIFI_EVENT_CONNECT, NULL, 0, pdMS_TO_TICKS(10));
        qmsd_wifi_sta_set_status(STA_CONNECTED);
        connect_num = 0;
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_event_sta_disconnected_t *disconnected = (wifi_event_sta_disconnected_t*) event_data;
        ESP_LOGE(TAG, "Disconnect reason: %d", disconnected->reason);
        disconnect_reason = disconnected->reason;
        connect_num += 1;
        switch (disconnected->reason) {
            case WIFI_REASON_AUTH_EXPIRE:
            case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:
            case WIFI_REASON_AUTH_FAIL:
            case WIFI_REASON_ASSOC_EXPIRE:
            case WIFI_REASON_HANDSHAKE_TIMEOUT:
            case WIFI_REASON_MIC_FAILURE: {
                ESP_LOGE(TAG, "STA Auth Error");
                if (connect_num <= auth_num || auth_num < 0) {
                    ESP_LOGI(TAG, "Disconnected. Connecting to the AP (%" PRIi32 ") again...", connect_num);
                    qmsd_wifi_sta_set_status(STA_RECONNECT);
                } else {
                    connect_num = 0;
                    qmsd_wifi_sta_set_status(STA_CONNECT_FAILED);
                    esp_event_post(QMSD_WIFI_EVENT, QMSD_WIFI_EVENT_DISCONNECT, event_data, sizeof(wifi_event_sta_disconnected_t), pdMS_TO_TICKS(10));
                }
                break;
            }
            case WIFI_REASON_NO_AP_FOUND: {
                ESP_LOGE(TAG, "STA AP Not found");
                if (connect_num <= ap_not_found_num || ap_not_found_num < 0) {
                    ESP_LOGI(TAG, "Disconnected. Connecting to the AP (%" PRIi32 ") again...", connect_num);
                    qmsd_wifi_sta_set_status(STA_RECONNECT);
                } else {
                    qmsd_wifi_sta_set_status(STA_CONNECT_FAILED);
                    esp_event_post(QMSD_WIFI_EVENT, QMSD_WIFI_EVENT_DISCONNECT, event_data, sizeof(wifi_event_sta_disconnected_t), pdMS_TO_TICKS(10));
                    connect_num = 0;
                }
                break;
            }
            case WIFI_REASON_ASSOC_LEAVE:
            default: {
                if (qmsd_wifi_sta_get_status() == STA_NOT_CONNECTED) {
                    connect_num = 0;
                    return ;
                }
                if (connect_num <= other_num || other_num < 0) {
                    ESP_LOGI(TAG, "Disconnected. Connecting to the AP (%" PRIi32 ") again...", connect_num);
                    qmsd_wifi_sta_set_status(STA_RECONNECT);
                } else {
                    esp_event_post(QMSD_WIFI_EVENT, QMSD_WIFI_EVENT_DISCONNECT, event_data, sizeof(wifi_event_sta_disconnected_t), pdMS_TO_TICKS(10));
                    qmsd_wifi_sta_set_status(STA_CONNECT_FAILED);
                    connect_num = 0;
                }
            }
        }

        if (connect_num % 10 == 0) {
            ESP_LOGE(TAG, "STA reconnect to much, restart wifi...");
            esp_wifi_stop();
            vTaskDelay(pdMS_TO_TICKS(10));
            esp_wifi_start();
            // esp_wifi_restore();
            // wifi_config_t config;
            // qmsd_wifi_sta_get_config(&config);
            // esp_wifi_set_config(ESP_IF_WIFI_STA, &config);
        }

        if (connect_num > 0) {
            uint32_t time_ms = connect_num * 500;
            if (time_ms > 15000) {
                time_ms = 15000;
            }
            // ESP_LOGE(TAG, "reconnect wifi wait %d ms", time_ms);
            TimerHandle_t timer = xTimerCreate("wifi_re", pdMS_TO_TICKS(time_ms), false, NULL, qmsd_wifi_connect_timer);
            xTimerStart(timer, 10);
        }
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) {
        ESP_LOGI(TAG, "STA Connecting to the AP again...");
    }
}

void qmsd_wifi_sta_init() {
    if (qmsd_wifi_event_group) {
        return ;
    }

    qmsd_wifi_init();

    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    qmsd_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &qmsd_wifi_sta_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &qmsd_wifi_sta_event_handler, NULL));
    qmsd_wifi_sta_set_status(STA_NOT_CONNECTED);
}

qmsd_wifi_status_t qmsd_wifi_sta_connect(wifi_config_t* conf, wifi_bandwidth_t bandwidth, uint32_t timeout) {
    if (conf == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (qmsd_wifi_event_group == NULL) {
        qmsd_wifi_sta_init();
    }

    // if mode == WIF_MODE_APSTA or WIFI_MODE_STA not change mode
    wifi_mode_t mode = 0;
    esp_wifi_get_mode(&mode);

    if (mode != WIFI_MODE_NULL || mode == WIFI_MODE_STA) {
        esp_wifi_set_mode(WIFI_MODE_STA);
    } else if (mode == WIFI_MODE_AP) {
        esp_wifi_set_mode(WIFI_MODE_APSTA);
    }

    if (qmsd_wifi_sta_get_status() == STA_CONNECTED) {
        return STA_CONNECTED;
    }

    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, conf));
    esp_wifi_set_bandwidth(ESP_IF_WIFI_STA, bandwidth);
    qmsd_wifi_sta_set_status(STA_CONNECTING);
    ESP_ERROR_CHECK(esp_wifi_start());
    esp_wifi_connect();
    memcpy(&sta_config, conf, sizeof(wifi_config_t));
    xEventGroupWaitBits(qmsd_wifi_event_group,  QMSD_WIFI_STA_CONNECTED | QMSD_WIFI_STA_CONNECT_FAILED, false, false, pdMS_TO_TICKS(timeout));
    return qmsd_wifi_sta_get_status();
}

void qmsd_wifi_sta_clear_connect_status() {
    if (qmsd_wifi_event_group == NULL) {
        return ;
    }
    xEventGroupClearBits(qmsd_wifi_event_group, QMSD_WIFI_STA_CONNECTED | QMSD_WIFI_STA_CONNECT_FAILED);
}

esp_err_t qmsd_wifi_sta_reconnect() {
    esp_wifi_stop();
    vTaskDelay(pdMS_TO_TICKS(20));
    qmsd_wifi_sta_set_status(STA_RECONNECT);
    esp_wifi_start();
    esp_wifi_connect();
    return ESP_OK;
}

void qmsd_wifi_sta_disconnect() {
    qmsd_wifi_sta_set_status(STA_NOT_CONNECTED);
    esp_wifi_disconnect();
}

qmsd_wifi_status_t qmsd_wifi_sta_wait_connect(uint32_t timeout) {
    if (qmsd_wifi_event_group == NULL) {
        return STA_NOT_CONNECTED;
    }
    xEventGroupWaitBits(qmsd_wifi_event_group, QMSD_WIFI_STA_CONNECTED | QMSD_WIFI_STA_CONNECT_FAILED, false, false, pdMS_TO_TICKS(timeout));
    return qmsd_wifi_sta_get_status();
}

qmsd_wifi_status_t qmsd_wifi_sta_wait_connect_success(uint32_t timeout) {
    if (qmsd_wifi_event_group == NULL) {
        return STA_NOT_CONNECTED;
    }
    xEventGroupWaitBits(qmsd_wifi_event_group, QMSD_WIFI_STA_CONNECTED, false, false, pdMS_TO_TICKS(timeout));
    return qmsd_wifi_sta_get_status();
}

// nums < 0 will reconnect always
void qmsd_wifi_sta_set_reconnect_times(int32_t auth, int32_t ap_not_found, int32_t other) {
    auth_num = auth;
    ap_not_found_num = ap_not_found;
    other_num = other;
}

int8_t qmsd_wifi_sta_get_rssi() {
    if (qmsd_wifi_sta_get_status() != STA_CONNECTED) {
        return -127;
    }

    wifi_ap_record_t ap_info;
    esp_wifi_sta_get_ap_info(&ap_info);
    return ap_info.rssi;
}

const char *qmsd_wifi_sta_get_ip() {
    return g_ip_addr;
}

qmsd_wifi_status_t qmsd_wifi_sta_get_config(wifi_config_t* config) {
    if (config != NULL) {
        memcpy(config, &sta_config, sizeof(wifi_config_t));
    }
    return qmsd_wifi_sta_get_status();
}

wifi_err_reason_t qmsd_wifi_sta_get_failed_reason() {
    return disconnect_reason;
}
