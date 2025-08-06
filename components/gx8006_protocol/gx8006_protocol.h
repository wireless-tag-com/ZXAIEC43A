#pragma once

#include <stdint.h>
#include "driver/uart.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t cmd;
    uint8_t* buffer;
    uint32_t len;
} gx8006_uart_frame_t;

void gx8006_protocol_init(uint8_t uart_num, uint8_t tx_pin, uint8_t rx_pin, uint32_t baudrate);

void gx8006_protocol_write_bytes_multi(uint8_t cmd, uint8_t frame_nums, ...);

void gx8006_protocol_write_bytes(uint8_t cmd, const uint8_t* frame, uint32_t len);

void gx8006_protocol_write_byte(uint8_t cmd, uint8_t data);

// 从队列中取一帧数据
int gx8006_protocol_recv_frame(gx8006_uart_frame_t* frame, uint32_t timeout_ms);

void gx8006_protocol_free_frame_buffer(gx8006_uart_frame_t* frame);

void gx8006_protocol_wait_write_done();

#ifdef __cplusplus
}
#endif
