// Copyright 2020 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_smartconfig.h>
#include <smartconfig_ack.h>
#include <wifi_provisioning/manager.h>
#include "qmsd_utils.h"

static const char *TAG  = "qmsd_smartconfig";

static bool g_prov_smartconfig_start = true;
static QueueHandle_t wifi_queue;

/* Event handler for catching system events */
static void smartconfig_event_handler(void *arg, esp_event_base_t event_base, int32_t  event_id, void *event_data) {
    switch (event_id) {
        case SC_EVENT_SCAN_DONE:
            ESP_LOGI(TAG, "Scan done");
            break;

        case SC_EVENT_FOUND_CHANNEL:
            ESP_LOGI(TAG, "Found channel");
            break;

        case SC_EVENT_GOT_SSID_PSWD: {
            wifi_config_t wifi_config = {0};
            smartconfig_event_got_ssid_pswd_t *evt = (smartconfig_event_got_ssid_pswd_t *)event_data;
            ESP_LOGI(TAG, "Got SSID and password, ssid: %s, password: %s", evt->ssid, evt->password);
            memcpy(wifi_config.sta.ssid, evt->ssid, sizeof(wifi_config.sta.ssid));
            memcpy(wifi_config.sta.password, evt->password, sizeof(wifi_config.sta.password));
            memset(evt->cellphone_ip, 0xff, sizeof(evt->cellphone_ip));
            xQueueSend(wifi_queue, &wifi_config, 0);
            break;
        }

        case SC_EVENT_SEND_ACK_DONE :
            ESP_LOGI(TAG, "SC_EVENT_SEND_ACK_DONE");
            break;

        default:
            break;
    }
}

esp_err_t qmsd_prov_smartconfig_start(smartconfig_type_t type) {
    esp_err_t err = ESP_OK;
    g_prov_smartconfig_start = true;
    if (wifi_queue == NULL) {
        wifi_queue = xQueueCreate(1, sizeof(wifi_config_t));
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_smartconfig_set_type(type));
    smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_smartconfig_start(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_register(SC_EVENT, ESP_EVENT_ANY_ID, &smartconfig_event_handler, NULL));

    return err;
}

esp_err_t qmsd_prov_smartconfig_stop() {
    esp_err_t err = ESP_OK;

    if (g_prov_smartconfig_start) {
        ESP_LOGI(TAG, "smartconfig over");
        err = esp_smartconfig_stop();
        QMSD_ERROR_CHECK(err != ESP_OK, err, "esp_smartconfig_stop");
        g_prov_smartconfig_start = false;
    }

    if (wifi_queue) {
        vQueueDelete(wifi_queue);
    }
    return err;
}

esp_err_t qmsd_prov_smartconfig_get_result(wifi_config_t* wifi_config, uint32_t timeout) {
    if (g_prov_smartconfig_start == false) {
        return ESP_FAIL;
    }

    QMSD_PARAM_CHECK(wifi_config);
    if (xQueuePeek(wifi_queue, wifi_config, timeout)) {
        return ESP_OK;
    }

    return ESP_ERR_TIMEOUT;
}
