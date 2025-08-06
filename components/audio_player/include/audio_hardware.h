#pragma once

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file audio_hardware.h
 * @brief ESP32音频硬件抽象层接口
 * 
 * 该文件定义了音频硬件的底层操作接口，包括音频数据的读写、
 * 音量控制、静音控制等功能。为上层音频播放器提供硬件抽象。
 * 
 * @author ESP32 Audio Team
 * @date 2024
 */

/**
 * @brief 初始化音频硬件
 * 
 * 初始化音频硬件设备，包括I2S、编解码器等
 */
void audio_hardware_init(void);

/**
 * @brief 向音频硬件写入数据
 * 
 * 将音频数据写入到硬件播放缓冲区
 * 
 * @param data 音频数据缓冲区指针
 * @param len 数据长度（字节）
 * @return int 实际写入的字节数，负值表示错误
 */
int audio_hardware_data_write(char* data, int len);

/**
 * @brief 从音频硬件读取数据
 * 
 * 从硬件录音缓冲区读取音频数据
 * 
 * @param data 存储音频数据的缓冲区指针
 * @param len 要读取的数据长度（字节）
 * @return int 实际读取的字节数，负值表示错误
 */
int audio_hardware_data_read(char* data, int len);

/**
 * @brief 设置音频音量
 * 
 * 设置音频输出的音量大小
 * 
 * @param volume 音量值，范围依赖于具体硬件实现
 */
void audio_hardware_set_volume(int volume);

/**
 * @brief 获取当前音量
 * 
 * @return int 当前音量值
 */
int audio_hardware_get_volume();

/**
 * @brief 增加音量
 * 
 * 在当前音量基础上增加指定的音量值
 * 
 * @param volume 要增加的音量值，可以为负值（减小音量）
 */
void audio_hardware_add_volume(int volume);

/**
 * @brief 静音控制
 * 
 * 控制音频输出的静音状态
 * 
 * @param mute 静音控制：1-静音，0-取消静音
 */
void audio_hardware_mute(int mute);

/**
 * @brief 重置写入缓冲区
 * 
 * 清空音频硬件的写入缓冲区，丢弃所有待播放的数据
 */
void audio_hardware_write_reset();

/**
 * @brief 获取剩余缓冲区大小
 * 
 * 获取音频硬件写入缓冲区的剩余空间大小
 * 
 * @return int 剩余缓冲区大小（字节）
 */
int audio_hardware_get_remaining_size();

#ifdef __cplusplus
}
#endif
