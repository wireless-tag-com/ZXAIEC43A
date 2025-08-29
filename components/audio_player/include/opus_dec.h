#pragma once

#include "esp_opus_dec.h"
#include "freertos/FreeRTOS.h"
#include "freertos/message_buffer.h"
#include "freertos/event_groups.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file opus_dec.h
 * @brief OPUS音频解码器接口
 * 
 * 该文件定义了OPUS音频解码器的接口和数据结构。
 * 支持实时OPUS音频流的解码，提供缓冲区管理和异步解码功能。
 * 
 * @author ESP32 Audio Team
 * @date 2024
 */

/**
 * @brief OPUS解码器句柄类型前向声明
 */
typedef struct _opus_dec_handle_t opus_dec_handle_t;

/**
 * @brief OPUS解码数据写入回调函数类型定义
 * 
 * 当解码器完成音频数据解码后，通过此回调函数输出PCM数据
 * 
 * @param decoder 解码器句柄
 * @param buffer 解码后的PCM数据缓冲区
 * @param length 数据长度（字节）
 * @param user_data 用户自定义数据指针
 */
typedef void (*opus_dec_write_cb)(opus_dec_handle_t* decoder, uint8_t* buffer, uint32_t length, void* user_data);

/**
 * @brief OPUS解码器句柄结构体
 * 
 * 包含解码器的所有运行时状态和配置信息
 */
typedef struct _opus_dec_handle_t {
    void* opus_handle;                /**< OPUS解码器内部句柄 */
    void* user_data;                  /**< 用户自定义数据指针 */
    opus_dec_write_cb write_cb;       /**< 数据写入回调函数 */
    MessageBufferHandle_t rb_handle;  /**< 环形缓冲区句柄 */
    EventGroupHandle_t event_group;   /**< 事件组句柄 */

    uint32_t buffer_packet_count;     /**< 缓冲区需要的包数量 */
    uint32_t current_packet_count;    /**< 当前已接收的包数量 */
    bool buffering_enabled;           /**< 是否启用缓冲区 */
    int software_gain_percent;        /**< 软件增益百分比，默认100% */
} opus_dec_handle_t;

/**
 * @brief 初始化OPUS解码器
 * 
 * 创建并初始化OPUS解码器实例
 * 
 * @param sample_rate 音频采样率（Hz）
 * @param rb_size 环形缓冲区大小（字节）
 * @return opus_dec_handle_t* 解码器句柄，NULL表示初始化失败
 */
opus_dec_handle_t* opus_dec_init(uint32_t sample_rate, uint32_t rb_size);

/**
 * @brief 向解码器发送OPUS数据
 * 
 * 将压缩的OPUS数据发送给解码器进行解码
 * 
 * @param handle 解码器句柄
 * @param data OPUS压缩数据缓冲区
 * @param len 数据长度（字节）
 * @param timeout_ms 发送超时时间（毫秒）
 * @return int 发送的字节数，负值表示错误
 */
int opus_dec_send_data(opus_dec_handle_t* handle, uint8_t* data, uint32_t len, uint32_t timeout_ms);

/**
 * @brief 设置写入完成标志
 * 
 * 通知解码器数据输入已完成，可以开始最终的解码处理
 * 
 * @param handle 解码器句柄
 */
void opus_dec_set_write_done(opus_dec_handle_t* handle);

/**
 * @brief 停止解码器
 * 
 * 停止解码器的运行，但不销毁解码器实例
 * 
 * @param handle 解码器句柄
 * @param timeout_ms 停止超时时间（毫秒）
 */
void opus_dec_stop(opus_dec_handle_t* handle, uint32_t timeout_ms);

/**
 * @brief 停止并销毁解码器
 * 
 * 停止解码器运行并释放所有相关资源
 * 
 * @param handle 解码器句柄
 */
void opus_dec_stop_and_deinit(opus_dec_handle_t* handle);

/**
 * @brief 设置缓冲区包数量
 * 
 * 设置解码器缓冲区需要缓存的数据包数量
 * 
 * @param handle 解码器句柄
 * @param packet_count 需要缓存的包数量
 */
void opus_dec_set_buffer_packet_count(opus_dec_handle_t* handle, uint32_t packet_count);

/**
 * @brief 等待解码完成
 * 
 * 阻塞等待解码器完成所有数据的解码处理
 * 
 * @param handle 解码器句柄
 * @param timeout_ms 等待超时时间（毫秒）
 * @return bool true-解码完成，false-超时
 */
bool opus_dec_wait_done(opus_dec_handle_t* handle, uint32_t timeout_ms);

/**
 * @brief 设置写入回调函数
 * 
 * 设置解码完成后的数据输出回调函数
 * 
 * @param handle 解码器句柄
 * @param write_cb 写入回调函数指针
 * @param user_data 传递给回调函数的用户数据
 */
void opus_dec_set_write_cb(opus_dec_handle_t* handle, opus_dec_write_cb write_cb, void* user_data);

/**
 * @brief 设置OPUS解码器软件增益
 * 
 * 通过软件算法对OPUS解码后的音频数据进行音量放大，可以突破硬件音量限制
 * 
 * @param handle 解码器句柄
 * @param gain_percent 增益百分比，100表示原音量，200表示放大2倍，50表示减半
 */
void opus_dec_set_software_gain(opus_dec_handle_t* handle, int gain_percent);

/**
 * @brief 获取OPUS解码器当前软件增益设置
 * 
 * @param handle 解码器句柄
 * @return int 当前软件增益百分比，如果handle为NULL则返回100
 */
int opus_dec_get_software_gain(opus_dec_handle_t* handle);

#ifdef __cplusplus
}
#endif
