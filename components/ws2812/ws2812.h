#pragma once

void ws2812_set_rainbow();

void ws2812_set_color(uint32_t color);

// void ws2812_breathe(uint32_t color, int total_duration_ms);

void ws2812_animate_gradient(uint32_t start_color, uint32_t end_color, int total_duration_ms);

void ws2812_blink(uint32_t color, bool on);

void ws2812_init(uint8_t gpio_num); 