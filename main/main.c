#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "driver/uart.h"
#include "esp_littlefs.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#include "audio_player_user.h"
#include "qmsd_board_pin.h"
#include "qmsd_button.h"
#include "qmsd_network.h"
#include "qmsd_ota.h"
#include "qmsd_utils.h"
#include "qmsd_wifi_sta.h"
#include "storage_nvs.h"

#include "aiha_ai_chat.h"
#include "aiha_http_common.h"
#include "audio_hardware.h"
#include "aiha_audio_http.h"
#include "chat_asr_ctrl.h"
#include "chat_notify.h"
#include "gx8006.h"

#define TAG "MAIN"

bool g_ecrypt_index = 0;

uint8_t vol_status = 40;
int volume_hope = -1;
btn_handle_t btn;

void littlefs_init(void) {
    esp_vfs_littlefs_conf_t conf = {
        .base_path = "/littlefs",
        .partition_label = "res",
        .format_if_mount_failed = true,
        .dont_mount = false,
    };
    esp_err_t ret = esp_vfs_littlefs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find LittleFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize LittleFS (%s)", esp_err_to_name(ret));
        }
        return;
    }

    size_t total = 0, used = 0;
    ret = esp_littlefs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get LittleFS partition information (%s)", esp_err_to_name(ret));
        esp_littlefs_format(conf.partition_label);
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }
}

void btn_callback_cb(btn_handle_t handle, void* user_data) {
    if (qmsd_button_get_repeat(handle) > 3) {
        storage_nvs_erase_key("wifiCfg");
        esp_restart();
    } else if (qmsd_button_get_repeat(handle) == 1) {
        if (qmsd_wifi_sta_get_status() == STA_NOT_CONNECTED) {
            aiha_chat_deal_error(ALLINONE_ERROR_CODE_HTTP_ERROR);
            return;
        }
        if (aiha_websocket_is_connected() == false || qmsd_wifi_sta_get_status() != STA_CONNECTED) {
            return;
        }
        if (gx8006_in_wakeup()) {
            aiha_chat_deal_error(ALLINONE_ERROR_CODE_USER_EXIT);
        }
    }

    ESP_LOGE(TAG, "btn repeat %d", qmsd_button_get_repeat(handle));
}

void aiha_tts_cb(const char* url, void* user_data) {
    ESP_LOGI(TAG, "tts url: %s", url);
    audio_player_play_url(url, 1);
}

void app_main(void) {
    printf("   ___    __  __   ____    ____  \n");
    printf("  / _ \\  |  \\/  | / ___|  |  _ \\ \n");
    printf(" | | | | | |\\/| | \\___ \\  | | | |\n");
    printf(" | |_| | | |  | |  ___) | | |_| |\n");
    printf("  \\__\\_\\ |_|  |_| |____/  |____/  \n");

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("GX8006", ESP_LOG_INFO);
    esp_log_level_set("aiha.allinOne", ESP_LOG_INFO);
    esp_log_level_set("HTTP_STREAM", ESP_LOG_ERROR);
    esp_log_level_set("AUDIO_PIPELINE", ESP_LOG_ERROR);
    esp_log_level_set("AUDIO_ELEMENT", ESP_LOG_ERROR);
    esp_log_level_set("AUDIO_THREAD", ESP_LOG_ERROR);
    esp_log_level_set("i2s_std", ESP_LOG_DEBUG);

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    printf("QMSD Start, version: " SOFT_VERSION "\n");
    gx8006_init(UART_NUM_1, EXT_UART_TXD_PIN, EXT_UART_RXD_PIN, EXT_UART_STA_PIN, EXT_AUDIO_RST_PIN, 921600);
    gx8006_set_audio_recv_callback(aiha_audio_recv_callback);
    ESP_LOGI(TAG, "gx8006 startup wait start");
    gx8006_wait_startup(portMAX_DELAY);
    ESP_LOGI(TAG, "gx8006 startup wait done");

    void led_status_init();
    led_status_init();

    qmsd_button_config_t btn_config = QMSD_BUTTON_DEFAULT_CONFIG;
    btn_config.debounce_ticks = 2;
    btn_config.short_ticks = 400 / btn_config.ticks_interval_ms;
    btn_config.update_task.core = 0;
    btn_config.update_task.priority = 13;
    btn_config.update_task.stack = 2 * 1024;
    qmsd_button_init(&btn_config);

    aiha_http_set_production_id("C38006");
    aiha_request_tts_set_cb(aiha_tts_cb);

    storage_nvs_init();
    littlefs_init();
    audio_hardware_init();
    audio_player_init();
    chat_notify_init();


    btn = qmsd_button_create_gpio(KEY_0_PIN, 0, NULL);
    qmsd_button_register_cb(btn, BUTTON_PRESS_DOWN, btn_callback_cb);
    qmsd_button_start(btn);

    uint8_t* volume_index = NULL;
    uint32_t lenv = sizeof(uint8_t);
    if (storage_nvs_read_blob("volume", (void**)&volume_index, &lenv) == ESP_OK) {
        vol_status = *volume_index;
    } else {
        vol_status = 60;
    }
    printf("vol_status: %d \n", vol_status);

    audio_hardware_set_volume(vol_status);

    chat_notify_audio_play(NOTIFY_STARTUP, NULL);
    qmsd_network_start(aiha_ai_chat_start);

    int volume_diff_count = 0;
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(40));

        char input = '\0';
        scanf("%c", &input);
        if (input == 'm') {
            qmsd_debug_heap_print(MALLOC_CAP_INTERNAL, 0);
        } else if (input == 'd') {
            qmsd_debug_task_print(0);
        } else if (input == 'p') {
            ESP_LOGI(TAG, "play");
            audio_player_play_url(MP3_URL_FROM_FILE "/littlefs/wifi_ap_not_found.mp3", 1);
            // 自动拼接 从"file:/" "/spiffs/wifi_failed.mp3"=>"file:/spiffs/wifi_failed.mp3"
        } else if (input == 'e') {
            ESP_LOGE(TAG, "stop");
            audio_player_stop_speak();  // 清除mp3流
            ESP_LOGE(TAG, "stop finish");
        } else if (input == 'c') {

        } else if (input == 'a') {
            ESP_LOGE(TAG, "audio_player_get_remaining_size: %d", audio_player_get_remaining_size());
        }

        if (vol_status != audio_hardware_get_volume()) {
            volume_diff_count += 1;
        } else {
            volume_diff_count = 0;
        }

        if (volume_diff_count > 100) {
            vol_status = audio_hardware_get_volume();
            storage_nvs_write_blob("volume", &vol_status, sizeof(vol_status));
            ESP_LOGI(TAG, "write to flash new volume: %d", vol_status);
            volume_diff_count = 0;
        }
    }
}
