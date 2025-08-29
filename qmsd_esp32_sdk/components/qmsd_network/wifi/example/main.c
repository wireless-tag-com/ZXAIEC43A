#include <stdio.h>
#include "string.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "qmsd_utils.h"
#include "qmsd_prov_wifi.h"
#include "qmsd_wifi_sta.h"

#define TAG "QMSD_WIFI"

#define TEST_SSID "SP-RD-2G"
#define TEST_PWD "SP12345678"

esp_err_t wifi_connect()
{
    // 基本wifi连接
    wifi_config_t conf = {
        .sta.ssid = TEST_SSID,
        .sta.password = TEST_PWD
    };
    qmsd_wifi_status_t ret = qmsd_wifi_sta_connect(&conf, WIFI_BW_HT20, portMAX_DELAY);
    QMSD_ERROR_CHECK(ret != STA_CONNECTED, ESP_FAIL, "Wifi connect %s failed", (char *)conf.sta.ssid);
    ESP_LOGI(TAG, "Wifi Connect success");

    // 获取已连接的Wi-Fi RSSI强度
    ESP_LOGI(TAG, "%s RSSI: %d", (char *)conf.sta.ssid, qmsd_wifi_sta_get_rssi());
    vTaskDelay(pdMS_TO_TICKS(4000));
    qmsd_wifi_sta_disconnect();
    ESP_LOGI(TAG, "Wifi disconnect by user");

    // 连接到找不到的AP, 设置尝试重连次数
    char* new_sta_ssid = "Not-AP -_-";
    char* new_sta_pwd = TEST_PWD;
    memcpy(conf.sta.ssid, new_sta_ssid, strlen(new_sta_ssid) + 1);
    memcpy(conf.sta.password, new_sta_pwd, strlen(new_sta_pwd) + 1);

    // 密码错误重连5次, 找不到AP重连4次, 其它错误无限重连
    qmsd_wifi_sta_set_reconnect_times(3, 3, -1);
    // 不进行等待
    qmsd_wifi_sta_connect(&conf, WIFI_BW_HT20, 0);
    qmsd_wifi_status_t wifi_status = qmsd_wifi_sta_wait_connect(portMAX_DELAY);
    if (wifi_status == STA_CONNECT_FAILED) {
        ESP_LOGE(TAG, "Connect to %s failed, reason: %d", (char *)conf.sta.ssid, qmsd_wifi_sta_get_failed_reason());
    }

    // 连接中途打断
    ESP_LOGI(TAG, "Test break when connecting");
    qmsd_wifi_sta_connect(&conf, WIFI_BW_HT20, 0);
    vTaskDelay(pdMS_TO_TICKS(5000));
    qmsd_wifi_sta_disconnect();

    // 连接新的Wifi
    ESP_LOGI(TAG, "Connect to new wifi");
    new_sta_ssid = TEST_SSID;
    new_sta_pwd = TEST_PWD;
    memcpy(conf.sta.ssid, new_sta_ssid, strlen(new_sta_ssid) + 1);
    memcpy(conf.sta.password, new_sta_pwd, strlen(new_sta_pwd) + 1);
    ret = qmsd_wifi_sta_connect(&conf, WIFI_BW_HT20, portMAX_DELAY);
    QMSD_ERROR_CHECK(ret != STA_CONNECTED, ESP_FAIL, "Wifi connect %s faled", (char *)conf.sta.ssid);
    return ESP_OK;
}

void app_main(void)
{   
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("wifi", ESP_LOG_ERROR);

    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    qmsd_wifi_sta_init();
    wifi_connect();
}
