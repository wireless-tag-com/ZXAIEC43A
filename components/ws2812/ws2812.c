/*
 * SPDX-FileCopyrightText: 2021-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/rmt_tx.h"

#include "qmsd_wifi_sta.h"
#include "qmsd_wifi.h"
#include "ws2812.h"

static rmt_channel_handle_t led_chan = NULL;
static rmt_encoder_handle_t simple_encoder = NULL;
static rmt_transmit_config_t tx_config;

#define RMT_LED_STRIP_RESOLUTION_HZ 10000000 // 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)
#define RMT_LED_STRIP_GPIO_NUM 4

#define EXAMPLE_LED_NUMBERS 2

// 最大亮度控制 (0-255, 255为最大亮度)
#define WS2812_MAX_BRIGHTNESS 50

#define EXAMPLE_FRAME_DURATION_MS 20
#define EXAMPLE_ANGLE_INC_FRAME 0.02
#define EXAMPLE_ANGLE_INC_LED 0.3

static const char* TAG = "led_ws2312";

static uint8_t led_strip_pixels[EXAMPLE_LED_NUMBERS * 3];

static const rmt_symbol_word_t ws2812_zero = {
    .level0 = 1,
    .duration0 = 0.3 * RMT_LED_STRIP_RESOLUTION_HZ / 1000000, // T0H=0.3us
    .level1 = 0,
    .duration1 = 0.9 * RMT_LED_STRIP_RESOLUTION_HZ / 1000000, // T0L=0.9us
};

static const rmt_symbol_word_t ws2812_one = {
    .level0 = 1,
    .duration0 = 0.9 * RMT_LED_STRIP_RESOLUTION_HZ / 1000000, // T1H=0.9us
    .level1 = 0,
    .duration1 = 0.3 * RMT_LED_STRIP_RESOLUTION_HZ / 1000000, // T1L=0.3us
};

//reset defaults to 50uS
static const rmt_symbol_word_t ws2812_reset = {
    .level0 = 1,
    .duration0 = RMT_LED_STRIP_RESOLUTION_HZ / 1000000 * 50 / 2,
    .level1 = 0,
    .duration1 = RMT_LED_STRIP_RESOLUTION_HZ / 1000000 * 50 / 2,
};

static size_t encoder_callback(const void* data, size_t data_size, size_t symbols_written, size_t symbols_free, rmt_symbol_word_t* symbols, bool* done, void* arg) {
    // We need a minimum of 8 symbol spaces to encode a byte. We only
    // need one to encode a reset, but it's simpler to simply demand that
    // there are 8 symbol spaces free to write anything.
    if (symbols_free < 8) {
        return 0;
    }

    // We can calculate where in the data we are from the symbol pos.
    // Alternatively, we could use some counter referenced by the arg
    // parameter to keep track of this.
    size_t data_pos = symbols_written / 8;
    uint8_t* data_bytes = (uint8_t*)data;
    if (data_pos < data_size) {
        // Encode a byte
        size_t symbol_pos = 0;
        for (int bitmask = 0x80; bitmask != 0; bitmask >>= 1) {
            if (data_bytes[data_pos] & bitmask) {
                symbols[symbol_pos++] = ws2812_one;
            } else {
                symbols[symbol_pos++] = ws2812_zero;
            }
        }
        // We're done; we should have written 8 symbols.
        return symbol_pos;
    } else {
        //All bytes already are encoded.
        //Encode the reset, and we're done.
        symbols[0] = ws2812_reset;
        *done = 1; //Indicate end of the transaction.
        return 1;  //we only wrote one symbol
    }
}

// 彩虹追逐
void ws2812_set_rainbow() {
    if (led_chan == NULL || simple_encoder == NULL) {
        return;
    }
    static float offset = 0;
    for (int led = 0; led < EXAMPLE_LED_NUMBERS; led++) {
        float angle = offset + (led * EXAMPLE_ANGLE_INC_LED);
        const float color_off = (M_PI * 2) / 3;
        uint8_t green = (sin(angle + color_off * 0) * 127 + 128) * WS2812_MAX_BRIGHTNESS / 255;
        uint8_t red = (sin(angle + color_off * 1) * 127 + 128) * WS2812_MAX_BRIGHTNESS / 255;
        uint8_t blue = (sin(angle + color_off * 2) * 117 + 128) * WS2812_MAX_BRIGHTNESS / 255;
        led_strip_pixels[led * 3 + 0] = green;
        led_strip_pixels[led * 3 + 1] = red;
        led_strip_pixels[led * 3 + 2] = blue;
    }
    ESP_ERROR_CHECK(rmt_transmit(led_chan, simple_encoder, led_strip_pixels, sizeof(led_strip_pixels), &tx_config));
    ESP_ERROR_CHECK(rmt_tx_wait_all_done(led_chan, portMAX_DELAY));
    offset += EXAMPLE_ANGLE_INC_FRAME;
    if (offset > 2 * M_PI) {
        offset -= 2 * M_PI;
    }
}

// 单色灯
void ws2812_set_color(uint32_t color) {
    if (led_chan == NULL || simple_encoder == NULL) {
        return;
    }
    uint8_t red = ((color >> 16) & 0xFF) * WS2812_MAX_BRIGHTNESS / 255;
    uint8_t green = ((color >> 8) & 0xFF) * WS2812_MAX_BRIGHTNESS / 255;
    uint8_t blue = (color & 0xFF) * WS2812_MAX_BRIGHTNESS / 255;

    for (int led = 0; led < EXAMPLE_LED_NUMBERS; led++) {
        led_strip_pixels[led * 3 + 0] = green;
        led_strip_pixels[led * 3 + 1] = red; 
        led_strip_pixels[led * 3 + 2] = blue;
    }

    ESP_ERROR_CHECK(rmt_transmit(led_chan, simple_encoder, led_strip_pixels, sizeof(led_strip_pixels), &tx_config));
    ESP_ERROR_CHECK(rmt_tx_wait_all_done(led_chan, portMAX_DELAY));
}


// 单色呼吸灯
void ws2812_breathe(uint32_t color, int total_duration_ms) {
    if (led_chan == NULL || simple_encoder == NULL) {
        return;
    }

    const int max_steps = 255;                               // 定义最大步骤数以确保渐变平滑
    int step_delay_ms = total_duration_ms / (2 * max_steps); // 每一步的延时时间

    int step = 0;
    bool increasing = true;

    while (1) {
        uint8_t red = ((color >> 16) & 0xFF) * WS2812_MAX_BRIGHTNESS / 255;
        uint8_t green = ((color >> 8) & 0xFF) * WS2812_MAX_BRIGHTNESS / 255;
        uint8_t blue = (color & 0xFF) * WS2812_MAX_BRIGHTNESS / 255;

        uint8_t current_red = (step * red) / max_steps;
        uint8_t current_green = (step * green) / max_steps;
        uint8_t current_blue = (step * blue) / max_steps;

        for (int led = 0; led < EXAMPLE_LED_NUMBERS; led++) {
            led_strip_pixels[led * 3 + 0] = current_green;
            led_strip_pixels[led * 3 + 1] = current_red;
            led_strip_pixels[led * 3 + 2] = current_blue;
        }

        ESP_ERROR_CHECK(rmt_transmit(led_chan, simple_encoder, led_strip_pixels, sizeof(led_strip_pixels), &tx_config));
        ESP_ERROR_CHECK(rmt_tx_wait_all_done(led_chan, portMAX_DELAY));

        // 更新步进
        if (increasing) {
            step++;
            if (step >= max_steps) {
                increasing = false; // 开始淡出
            }
        } else {
            step--;
            if (step <= 0) {
                increasing = true; // 再次开始淡入
            }
        }

        // 延迟，用于控制呼吸效果的速度
        vTaskDelay(pdMS_TO_TICKS(step_delay_ms));
    }
}

// 灯光颜色渐变
void ws2812_animate_gradient(uint32_t start_color, uint32_t end_color, int total_duration_ms) {
    if (led_chan == NULL || simple_encoder == NULL) {
        return;
    }
    uint8_t start_red = ((start_color >> 16) & 0xFF) * WS2812_MAX_BRIGHTNESS / 255;
    uint8_t start_green = ((start_color >> 8) & 0xFF) * WS2812_MAX_BRIGHTNESS / 255;
    uint8_t start_blue = (start_color & 0xFF) * WS2812_MAX_BRIGHTNESS / 255;

    uint8_t end_red = ((end_color >> 16) & 0xFF) * WS2812_MAX_BRIGHTNESS / 255;
    uint8_t end_green = ((end_color >> 8) & 0xFF) * WS2812_MAX_BRIGHTNESS / 255;
    uint8_t end_blue = (end_color & 0xFF) * WS2812_MAX_BRIGHTNESS / 255;

    const int max_steps = 255;                               // 定义最大步骤数以确保渐变平滑
    int step_delay_ms = total_duration_ms / (max_steps * 2); // 每一步的延时时间

    for (int step = 0; step <= max_steps; step++) {
        uint8_t current_red = start_red + (end_red - start_red) * step / max_steps;
        uint8_t current_green = start_green + (end_green - start_green) * step / max_steps;
        uint8_t current_blue = start_blue + (end_blue - start_blue) * step / max_steps;

        for (int led = 0; led < EXAMPLE_LED_NUMBERS; led++) {
            led_strip_pixels[led * 3 + 0] = current_green;
            led_strip_pixels[led * 3 + 1] = current_red;
            led_strip_pixels[led * 3 + 2] = current_blue;
        }

        ESP_ERROR_CHECK(rmt_transmit(led_chan, simple_encoder, led_strip_pixels, sizeof(led_strip_pixels), &tx_config));
        ESP_ERROR_CHECK(rmt_tx_wait_all_done(led_chan, portMAX_DELAY));

        if (step < max_steps) {
            vTaskDelay(pdMS_TO_TICKS(step_delay_ms));
        }
    }

    for (int step = max_steps; step > 0; step--) {
        uint8_t current_red = start_red + (end_red - start_red) * step / max_steps;
        uint8_t current_green = start_green + (end_green - start_green) * step / max_steps;
        uint8_t current_blue = start_blue + (end_blue - start_blue) * step / max_steps;

        for (int led = 0; led < EXAMPLE_LED_NUMBERS; led++) {
            led_strip_pixels[led * 3 + 0] = current_green;
            led_strip_pixels[led * 3 + 1] = current_red;
            led_strip_pixels[led * 3 + 2] = current_blue;
        }

        ESP_ERROR_CHECK(rmt_transmit(led_chan, simple_encoder, led_strip_pixels, sizeof(led_strip_pixels), &tx_config));
        ESP_ERROR_CHECK(rmt_tx_wait_all_done(led_chan, portMAX_DELAY));

        if (step > 1) {
            vTaskDelay(pdMS_TO_TICKS(step_delay_ms));
        }
    }
}

void ws2812_blink(uint32_t color, bool on) {
    if (led_chan == NULL || simple_encoder == NULL) {
        return;
    }

    uint8_t red = ((color >> 8) & 0xFF) * WS2812_MAX_BRIGHTNESS / 255;
    uint8_t green = ((color >> 16) & 0xFF) * WS2812_MAX_BRIGHTNESS / 255;
    uint8_t blue = (color & 0xFF) * WS2812_MAX_BRIGHTNESS / 255;

    // If "on" is true, set LEDs to the specified color; if false, turn LEDs off
    uint8_t current_red = on ? red : 0;
    uint8_t current_green = on ? green : 0;
    uint8_t current_blue = on ? blue : 0;

    // Update each LED to the chosen state (on or off)
    for (int led = 0; led < EXAMPLE_LED_NUMBERS; led++) {
        led_strip_pixels[led * 3 + 0] = current_red;
        led_strip_pixels[led * 3 + 1] = current_green;
        led_strip_pixels[led * 3 + 2] = current_blue;
    }

    // Transmit the updated LED colors
    ESP_ERROR_CHECK(rmt_transmit(led_chan, simple_encoder, led_strip_pixels, sizeof(led_strip_pixels), &tx_config));
    ESP_ERROR_CHECK(rmt_tx_wait_all_done(led_chan, portMAX_DELAY));
}

void ws2812_init(uint8_t gpio_num) {
    rmt_tx_channel_config_t tx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT, // select source clock
        .gpio_num = gpio_num,
        .mem_block_symbols = 64, // increase the block size can make the LED less flickering
        .resolution_hz = RMT_LED_STRIP_RESOLUTION_HZ,
        .trans_queue_depth = 4, // set the number of transactions that can be pending in the background
    };
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &led_chan));

    const rmt_simple_encoder_config_t simple_encoder_cfg = {.callback = encoder_callback};
    ESP_ERROR_CHECK(rmt_new_simple_encoder(&simple_encoder_cfg, &simple_encoder));

    ESP_ERROR_CHECK(rmt_enable(led_chan));

    tx_config.loop_count = 0; // no transfer loop
}
