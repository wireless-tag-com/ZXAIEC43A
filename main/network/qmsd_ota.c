#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>

#include "aiha_req_ota.h"
#include "cJSON.h"
#include "chat_notify.h"
#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "nvs.h"
#include "nvs_flash.h"

#define TAG "qmsd_iothub_ota"

EventGroupHandle_t g_ota_event_group;

#define OTA_WAIT_UPDATE (1 << 0)
#define OTA_START (1 << 1)
#define OTA_SUCCESS (1 << 2)
#define OTA_FAILED (1 << 3)

typedef struct {
    char url[512];
    uint8_t download_percent;
} iothub_ota_info_t;

static iothub_ota_info_t* g_info;

extern esp_err_t esp_crt_bundle_attach(void* conf);

static void ota_task(void* arg) {
    uint8_t retry = 3;
    iothub_ota_info_t* ota_info = (iothub_ota_info_t*)arg;

ota_restart:
    ESP_LOGI(TAG, "Starting Advanced OTA");
    esp_http_client_config_t config = {
        .url = ota_info->url,
        .timeout_ms = 600000,
        .buffer_size = 4096,
        .keep_alive_enable = true,
        .user_data = ota_info,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    esp_https_ota_config_t ota_config = {
        .http_config = &config,

    };

    esp_https_ota_handle_t https_ota_handle = NULL;
    esp_err_t err = esp_https_ota_begin(&ota_config, &https_ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ESP HTTPS OTA Begin failed");
        retry -= 1;
        if (retry > 0) {
            vTaskDelay(pdMS_TO_TICKS(2000));
            goto ota_restart;
        }
        xEventGroupSetBits(g_ota_event_group, OTA_FAILED);
        vTaskDelete(NULL);
    }

    esp_app_desc_t app_desc;
    err = esp_https_ota_get_img_desc(https_ota_handle, &app_desc);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_https_ota_read_img_desc failed");
        goto ota_end;
    }

    while (1) {
        err = esp_https_ota_perform(https_ota_handle);
        if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
            break;
        }
        if (esp_https_ota_get_image_size(https_ota_handle) != 0) {
            uint8_t now_percent = esp_https_ota_get_image_len_read(https_ota_handle) * 100 / esp_https_ota_get_image_size(https_ota_handle);
            if ((now_percent / 10) != (ota_info->download_percent / 10)) {
                ESP_LOGI(TAG, "Image bytes read: %d %%", now_percent);
            }
            ota_info->download_percent = now_percent;
        }
    }

    if (esp_https_ota_is_complete_data_received(https_ota_handle) != true) {
        ESP_LOGE(TAG, "Complete data was not received.");
        goto ota_end;
    }

    esp_err_t ota_finish_err = esp_https_ota_finish(https_ota_handle);
    if ((err == ESP_OK) && (ota_finish_err == ESP_OK)) {
        ESP_LOGI(TAG, "ESP_HTTPS_OTA upgrade successful. Wait rebooting ...");
        xEventGroupSetBits(g_ota_event_group, OTA_SUCCESS);
    } else {
        if (ota_finish_err == ESP_ERR_OTA_VALIDATE_FAILED) {
            ESP_LOGE(TAG, "Image validation failed, image is corrupted");
        }
        ESP_LOGE(TAG, "ESP_HTTPS_OTA upgrade failed 0x%x", ota_finish_err);
        xEventGroupSetBits(g_ota_event_group, OTA_FAILED);
    }
    xEventGroupClearBits(g_ota_event_group, OTA_START);
    vTaskDelete(NULL);

ota_end:
    esp_https_ota_abort(https_ota_handle);
    https_ota_handle = NULL;
    ESP_LOGE(TAG, "ESP_HTTPS_OTA upgrade failed");
    retry -= 1;
    if (retry > 0) {
        vTaskDelay(pdMS_TO_TICKS(2000));
        goto ota_restart;
    }
    xEventGroupSetBits(g_ota_event_group, OTA_FAILED);
    xEventGroupClearBits(g_ota_event_group, OTA_START);
    vTaskDelete(NULL);
}

esp_err_t ota_start(const char* url) {
    // printf("url:%s \n", url);
    g_ota_event_group = xEventGroupCreate();

    if (xEventGroupGetBits(g_ota_event_group) & OTA_START || url == NULL) {
        return ESP_FAIL;
    }
    if (g_info == NULL) {
        g_info = heap_caps_malloc(sizeof(iothub_ota_info_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    }
    g_info->download_percent = 0;
    strcpy(g_info->url, url);
    xEventGroupSetBits(g_ota_event_group, OTA_START);
    xEventGroupClearBits(g_ota_event_group, OTA_SUCCESS | OTA_FAILED);
    xTaskCreatePinnedToCore(ota_task, "ota_task", 4 * 1024, g_info, 7, NULL, 0);
    return ESP_OK;
}

void qmsd_ota_get_status(uint8_t* percent, uint8_t* status) {
    if (g_info == NULL) {
        *percent = 0;
        *status = 0;
        return;
    }
    if (g_ota_event_group == NULL) {
        *percent = 0;
        *status = 0;
    }
    *percent = g_info->download_percent;
    EventBits_t bits = xEventGroupGetBits(g_ota_event_group);
    if (bits & OTA_SUCCESS) {
        *status = 1;
    } else if (bits & OTA_FAILED) {
        *status = 2;
    } else {
        *status = 0;
    }
}

void qmsd_check_ota_by_http() {
    if (aiha_ota_req_url(SOFT_VERSION) != ESP_OK) {
        return;
    }
    chat_notify_audio_play(NOTIFY_OTA_START, NULL);
    vTaskDelay(pdMS_TO_TICKS(2000));
    ota_start(aiha_ota_get_url());
    uint8_t updata_state = 0;
    uint8_t ota_percent = 0;
    int8_t ota_status = 0;

    while (1) {
        qmsd_ota_get_status(&ota_percent, &updata_state);
        if (updata_state == 1) {
            chat_notify_audio_play(NOTIFY_OTA_SUCCESS, NULL);
            vTaskDelay(pdMS_TO_TICKS(2000));
            esp_restart();
            while (1) {
                vTaskDelay(pdMS_TO_TICKS(2000));
            }
        } else if (updata_state == 2) {
            if (ota_status != 2) {
                chat_notify_audio_play(NOTIFY_OTA_FAILED, NULL);
            }
            ota_status = 2;
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
