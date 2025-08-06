#include "aiha_websocket.h"
#include "audio_player_user.h"
#include "esp_log.h"
#include "gx8006.h"
#include "qmsd_board_pin.h"
#include "qmsd_button.h"
#include "qmsd_ota.h"
#include "qmsd_wifi_sta.h"
#include "ws2812.h"

#define TAG "LED"

typedef enum {
    LED_IDLE = 0x00,                // 待机状态(常亮)
    LED_AI_QUESTION = 0X01,         // 问问题阶段(1呼吸)
    LED_AI_ANSWER = 0X02,           // 回答问题阶段(呼吸)
    LED_NOT_PAIRED = 0x03,          // 未配对(红色 0.5s闪烁)
    LED_WIFI_NOT_CONNECTED = 0x04,  // wifi未连接(红色 呼吸)
    LED_WIFI_CONNECTING = 0x05,     // wifi连接中(红色到黄色呼吸)
    LED_OTA_UPDATING = 0x06,        // OTA升级中（蓝色呼吸）
    LED_OTA_SUCCESS = 0x07,         // OTA升级成功（绿色常亮）
    LED_OTA_FAILED = 0xF1,          // OTA升级失败（红色常亮）
} led_mode_t;

static led_mode_t led_mode = LED_NOT_PAIRED;
uint32_t custom_lighting = 0xC500FF;  // 定制灯光颜色
extern bool g_ecrypt_index;

void ws2812_task() {
    bool led_on = false;
    ws2812_init(LED_GPIO_PIN);
    // chat_cont_status_t now_chat_mode;
    uint16_t timer = 0;

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(50));
        uint8_t ota_percent = 0;
        uint8_t ota_status = 0;
        qmsd_ota_get_status(&ota_percent, &ota_status);

        if (qmsd_wifi_sta_get_status() != STA_CONNECTED) {
            if (qmsd_wifi_sta_get_status() == STA_CONNECTING) {
                led_mode = LED_WIFI_CONNECTING;
            } else if (qmsd_wifi_sta_get_status() == STA_NOT_CONNECTED) {
                led_mode = LED_NOT_PAIRED;
            } else {
                led_mode = LED_WIFI_NOT_CONNECTED;
            }
        } else {
            if (aiha_websocket_is_connected() == false) {
                led_mode = LED_NOT_PAIRED;
            } else {
                led_mode = LED_IDLE;
            }

            allinone_status_t status = aiha_websocket_get_status();
            if (gx8006_in_wakeup() == false) {
                led_mode = LED_IDLE;
            } else if (gx8006_in_wakeup() == true) {
                if (status < ALLINONE_STATUS_ASR_FINISH) {
                    led_mode = LED_AI_QUESTION;
                } else if (status >= ALLINONE_STATUS_ASR_FINISH && status <= ALLINONE_STATUS_FINISH) {
                    led_mode = LED_AI_ANSWER;
                } else if (status == ALLINONE_STATUS_ERROR) {
                    led_mode = LED_AI_ANSWER;
                }
            }

            if (ota_status == 1) {
                led_mode = LED_OTA_UPDATING;
            } else if (ota_status == 2) {
                led_mode = LED_OTA_SUCCESS;
            } else if (ota_status == 3) {
                led_mode = LED_OTA_FAILED;
            }
        }

        if (g_ecrypt_index == 0) {
            // led_mode = LED_NOT_PAIRED;
        }
        // ESP_LOGI("TAG", "opus_status:%d, led_mode:%d, wifi:%d %d", opus_status, led_mode, qmsd_wifi_sta_get_status());

        if (led_mode == LED_IDLE) {
            ws2812_set_color(custom_lighting);
        } else if (led_mode == LED_AI_QUESTION) {
            ws2812_animate_gradient(custom_lighting, 0x0000FF, 1000);
        } else if (led_mode == LED_AI_ANSWER) {
            ws2812_animate_gradient(custom_lighting, 0x00FFD6, 2000);
        } else if (led_mode == LED_NOT_PAIRED) {
            led_on = !led_on;
            ws2812_blink(0xff0000, led_on);
            vTaskDelay(pdMS_TO_TICKS(250));
        } else if (led_mode == LED_WIFI_NOT_CONNECTED) {
            ws2812_animate_gradient(0xFF0000, 0x000000, 2000);
        } else if (led_mode == LED_WIFI_CONNECTING) {
            ws2812_animate_gradient(0xFF0000, 0xFFDF0E, 2000);
        } else if (led_mode == LED_OTA_UPDATING) {
            if (ota_percent < 20)
                timer = 5000;
            else if (ota_percent < 40)
                timer = 4000;
            else if (ota_percent < 60)
                timer = 3000;
            else if (ota_percent < 80)
                timer = 2000;
            else if (ota_percent < 100)
                timer = 1000;
            ws2812_animate_gradient(0x0000FF, 0x000000, timer);
        } else if (led_mode == LED_OTA_FAILED) {
            ws2812_set_color(0xFF0000);
        } else if (led_mode == LED_OTA_SUCCESS) {
            ws2812_set_color(0x00FF00);
        }
    }

    vTaskDelete(NULL);
}

void led_status_init() {
    xTaskCreate(ws2812_task, "ws2812_task", 2 * 1024, NULL, 6, NULL);
}
