#include <stdio.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>

#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include "cJSON.h"

#include "qmsd_utils.h"
#include "qmsd_wifi_sta.h"

static const char *TAG  = "qmsd_softap";
QueueHandle_t ssid_queue;
TaskHandle_t scan_handle;
static bool g_prov_ap_start = false;
static QueueHandle_t wifi_queue;
const char* g_device_id = NULL;

static void udp_server_task(void *pvParameters) {
    esp_err_t err     = ESP_FAIL;
    char *rx_buffer   = malloc(128);
    socklen_t socklen = sizeof(struct sockaddr_in);
    struct sockaddr_in client_addr = {0};
    struct sockaddr_in server_addr = {
        .sin_addr.s_addr = htonl(INADDR_ANY),
        .sin_family      = AF_INET,
        .sin_port        = htons(8266),
    };

    int udp_server_sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    QMSD_ERROR_GOTO(udp_server_sockfd < 0, EXIT, "Unable to create socket, errno %d, err_str: %s", errno, strerror(errno));

    err = bind(udp_server_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    QMSD_ERROR_GOTO(err < 0, EXIT, "Socket unable to bind: errno %d, err_str: %s", errno, strerror(errno));

    ESP_LOGI(TAG, "Socket bound, port %d", 8266);
    uint8_t scan_first = 1;
    cJSON* recv_json;
    cJSON* recv_cmd_json;
    while (g_prov_ap_start) {
        fd_set read_fd;
        struct timeval timeout = {.tv_sec = 1,};
        FD_ZERO(&read_fd);
        FD_SET(udp_server_sockfd, &read_fd);
        err = select(udp_server_sockfd + 1, &read_fd, NULL, NULL, &timeout);
        QMSD_ERROR_GOTO(err < 0, EXIT, "recvfrom failed, errno %d, err_str: %s", errno, strerror(errno));
        QMSD_ERROR_CONTINUE(err == 0 || !FD_ISSET(udp_server_sockfd, &read_fd), "");
        memset(rx_buffer, 0, 128);
        int len = recvfrom(udp_server_sockfd, rx_buffer, 128, 0, (struct sockaddr *)&client_addr, &socklen);
        QMSD_ERROR_GOTO(len < 0, EXIT, "recvfrom failed, errno %d, err_str: %s", errno, strerror(errno));
        ESP_LOGI(TAG, "recvfrom, data: %s", rx_buffer);
        
        recv_json = cJSON_Parse(rx_buffer);
        recv_cmd_json = cJSON_GetObjectItem(recv_json, "cmd");
        QMSD_ERROR_CONTINUE(recv_cmd_json == NULL || !FD_ISSET(udp_server_sockfd, &read_fd), "");
        cJSON *out_json = cJSON_CreateObject();
        cJSON_AddNumberToObject(out_json, "status", 0x00);
        cJSON_AddStringToObject(out_json, "describe", "");
        if (recv_cmd_json->valueint == 0x81) {
            if (scan_first) {
                scan_first = 0;
            } else {
                vTaskResume(scan_handle);
            }
            cJSON *resolutions;
            xQueueReceive(ssid_queue, &resolutions, portMAX_DELAY);
            cJSON_AddNumberToObject(out_json, "cmd", 0x01);
            cJSON_AddItemToObject(out_json, "ssidList", resolutions);
        } else if (recv_cmd_json->valueint == 0x82) {
            cJSON* ssid_json = cJSON_GetObjectItem(recv_json, "ssid");
            cJSON* password_json = cJSON_GetObjectItem(recv_json, "password");
            if (ssid_json == NULL || password_json == NULL) {
                cJSON_AddNumberToObject(out_json, "status", 0x01);
                cJSON_AddStringToObject(out_json, "describe", "SSID或password未找到");
            } else {
                wifi_config_t wifi_config = {0};
                ESP_LOGI(TAG, "Got SSID and password, ssid: %s, password: %s", ssid_json->valuestring, password_json->valuestring);
                memcpy(wifi_config.sta.ssid, ssid_json->valuestring, strlen(ssid_json->valuestring) + 1);
                memcpy(wifi_config.sta.password, password_json->valuestring, strlen(password_json->valuestring) + 1);
                xQueueSend(wifi_queue, &wifi_config, 0);
            }
            cJSON_AddNumberToObject(out_json, "cmd", 0x02);
            cJSON_AddStringToObject(out_json, "deviceId", g_device_id);
        } else {
            cJSON_AddNumberToObject(out_json, "status", 0x01);
            cJSON_AddStringToObject(out_json, "describe", "未支持指令");
        }
        const char* out_data = cJSON_Print(out_json);
        for (uint8_t i = 0; i < 3; i++) {
            sendto(udp_server_sockfd, out_data, strlen(out_data), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
            vTaskDelay(pdMS_TO_TICKS(100));
        }

        if (recv_cmd_json->valueint == 0x81) {
            cJSON_AddNumberToObject(out_json, "cmd", 0x02);
            cJSON_AddStringToObject(out_json, "deviceId", g_device_id);
            out_data = cJSON_Print(out_json);
            for (uint8_t i = 0; i < 3; i++) {
                sendto(udp_server_sockfd, out_data, strlen(out_data), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
                vTaskDelay(pdMS_TO_TICKS(100));
            }
        }
    

        cJSON_Delete(out_json);
    }

EXIT:
    if (udp_server_sockfd != -1) {
        ESP_LOGI(TAG, "Shutting down socket");
        shutdown(udp_server_sockfd, 0);
        close(udp_server_sockfd);
    }

    vTaskDelete(NULL);
}

static void wifi_scan(void *pvParameters) {
    uint16_t number = 20;
    wifi_ap_record_t ap_info[20];
    uint16_t ap_count = 0;
    memset(ap_info, 0, sizeof(ap_info));

    ESP_LOGI(TAG, "Scan start");
    for (;;) {
        esp_wifi_scan_start(NULL, true);
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
        ESP_LOGI(TAG, "Total APs scanned = %u", ap_count);
        cJSON *resolutions = cJSON_CreateArray();
        for (int i = 0; (i < 20) && (i < ap_count); i++) {
            cJSON *n = cJSON_CreateString((char *)ap_info[i].ssid);
            cJSON_AddItemToArray(resolutions, n);
        }

        if (xQueueSend(ssid_queue, &resolutions, 0) == pdFALSE) {
            cJSON *json_delete;
            if (xQueueReceive(ssid_queue, &json_delete, 0) == pdTRUE) {
                cJSON_Delete(json_delete);
            }
            xQueueSend(ssid_queue, &resolutions, 0);
        }

        vTaskSuspend(NULL);
    }
}

esp_err_t qmsd_prov_softap_start(const char* ssid, const char* password, const char* device_id) {
    if (g_prov_ap_start) {
        return ESP_OK;
    }
    g_prov_ap_start = true;
    g_device_id = device_id;
    esp_netif_create_default_wifi_ap();
    wifi_config_t wifi_config = {
        .ap = {
            .max_connection = 5,
        },
    };

    strlcpy((char *) wifi_config.ap.ssid, ssid, sizeof(wifi_config.ap.ssid));

    if (strlen(password) == 0) {
        memset(wifi_config.ap.password, 0, sizeof(wifi_config.ap.password));
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    } else {
        strlcpy((char *) wifi_config.ap.password, password, sizeof(wifi_config.ap.password));
        wifi_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
    }

    /* Start WiFi in AP mode with configuration built above */
    esp_err_t err = esp_wifi_set_mode(WIFI_MODE_APSTA);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set WiFi mode : %d", err);
        return err;
    }
    err = esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set WiFi config : %d", err);
        return err;
    }
    err = esp_wifi_start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start WiFi : %d", err);
        return err;
    }

    ssid_queue = xQueueCreate(1, sizeof(cJSON *));
    wifi_queue = xQueueCreate(1, sizeof(wifi_config_t));

    xTaskCreatePinnedToCore(udp_server_task, "test task",  4 * 1024, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(wifi_scan, "wifi scan",  4 * 1024, NULL, 5, &scan_handle, 1);
    return err;
}

esp_err_t qmsd_prov_ap_stop() {
    esp_err_t err = ESP_OK;

    if (g_prov_ap_start) {
        g_prov_ap_start = false;
        esp_wifi_stop();
    }
    if (wifi_queue) {
        vQueueDelete(wifi_queue);
    }
    return err;
}

esp_err_t qmsd_prov_ap_get_result(wifi_config_t* wifi_config, uint32_t timeout) {
    if (g_prov_ap_start == false) {
        return ESP_FAIL;
    }

    QMSD_PARAM_CHECK(wifi_config);
    if (xQueuePeek(wifi_queue, wifi_config, timeout)) {
        vTaskDelay(pdMS_TO_TICKS(400));
        return ESP_OK;
    }

    return ESP_ERR_TIMEOUT;
}
