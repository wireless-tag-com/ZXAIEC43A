#pragma once

#include "esp_audio.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file audio_player_user.h
 * @brief ESP32音频播放器用户接口
 * 
 * 该文件定义了音频播放器的用户接口，支持MP3和OPUS格式的音频播放。
 * 提供了文件播放、原始数据播放、音频控制等功能。
 * 
 * @author ESP32 Audio Team
 * @date 2024
 */

/**
 * @brief 音频播放器任务优先级
 */
#define AUDIO_PLAYER_TASK_PRIO 8

/**
 * @brief 音频播放器任务运行的CPU核心
 */
#define AUDIO_PLAYER_TASK_CORE 0

/**
 * @brief MP3播放相关的URL标记定义
 */
#define MP3_FINISH_URL_MASK "End"    /**< 播放结束标记 */
#define MP3_URL_FROM_FILE "file:/"   /**< 从文件系统播放的标记 */
#define MP3_URL_FROM_RAW "raw:/"     /**< 从原始数据播放的标记 */
#define OPUS_URL_FROM_RAW "opus:/"   /**< 从原始OPUS数据播放的标记 */

/**
 * @brief 音频控制命令枚举
 */
typedef enum {
    AUDIO_CTRL_MUTE,     /**< 静音命令 */
    AUDIO_CTRL_UNMUTE,   /**< 取消静音命令 */
    AUDIO_CTRL_MAX,      /**< 最大枚举值，用于边界检查 */
} audio_ctrl_t;

/**
 * @brief 音频输出控制回调函数类型定义
 * 
 * 用于处理音频输出控制，如静音/取消静音操作
 * 
 * @param ctrl 控制命令，参见 audio_ctrl_t
 */
typedef void (*audio_output_ctrl_t)(audio_ctrl_t ctrl);

/**
 * @brief 音频数据写入回调函数类型定义
 * 
 * 用于接收解码后的音频数据
 * 
 * @param data 音频数据缓冲区指针
 * @param len 数据长度（字节）
 */
typedef void (*audio_data_write_t)(char* data, int len);

/**
 * @brief 获取音频播放器句柄
 * 
 * @return esp_audio_handle_t 音频播放器句柄
 */
esp_audio_handle_t audio_user_get_player(void);

/**
 * @brief 初始化音频播放器
 * 
 * 初始化音频播放器模块，包括硬件初始化、任务创建等
 */
void audio_player_init();

/**
 * @brief 设置音频输出控制回调函数
 * 
 * @param output_ctrl 输出控制回调函数指针
 */
void audio_player_set_output_ctrl(audio_output_ctrl_t output_ctrl);

/**
 * @brief 停止当前播放
 * 
 * 立即停止当前的音频播放
 */
void audio_player_stop_speak();

/**
 * @brief 获取剩余缓冲区大小
 * 
 * @return int 剩余缓冲区大小（字节）
 */
int audio_player_get_remaining_size();

/**
 * @brief 播放指定URL的音频
 * 
 * @param url 音频文件URL或标记
 * @param restart 是否重启播放：1-停止上一次播放，0-不停止
 */
void audio_player_play_url(const char* url, uint8_t restart);

/**
 * @brief 检查音频播放器是否正在运行
 * 
 * @return int 1-正在运行，0-未运行
 */
int audio_player_in_running(void);

/**
 * @brief 等待流水线播放完成
 * 
 * 阻塞等待当前音频播放完成
 */
void audio_player_wait_stream_pipeline_finish(void);

/**
 * @brief 等待流水线开始运行
 * 
 * 阻塞等待音频播放流水线开始运行
 */
void audio_player_wait_stream_pipeline_running(void);

/**
 * @brief 停止播放并等待完成
 * 
 * 停止当前播放并阻塞等待停止操作完成
 */
void audio_player_stop_and_wait(void);

/**
 * @brief 写入原始MP3数据
 * 
 * 向MP3播放器写入原始MP3数据，仅用于MP3播放器
 * 
 * @param buf 数据缓冲区指针
 * @param len 数据长度（字节）
 * @return esp_err_t ESP_OK-成功，其他-失败
 */
esp_err_t audio_player_raw_mp3_write(char* buf, int len);

/**
 * @brief 完成原始数据写入
 * 
 * 通知播放器原始数据写入完成
 * 
 * @return esp_err_t ESP_OK-成功，其他-失败
 */
esp_err_t audio_player_raw_write_finish(void);

/**
 * @brief 完成MP3播放并重启
 * 
 * 完成当前MP3播放并准备重新开始，仅用于MP3播放器
 */
void audio_player_raw_mp3_finish_and_restart(void);

/**
 * @brief 写入OPUS数据
 * 
 * 向OPUS播放器写入OPUS数据，仅用于OPUS播放器
 * 
 * @param buf 数据缓冲区指针
 * @param len 数据长度（字节）
 * @return esp_err_t ESP_OK-成功，其他-失败
 */
esp_err_t audio_player_opus_write(char* buf, int len);

/**
 * @brief 等待播放器空闲
 * 
 * 阻塞等待播放器进入空闲状态
 * 
 * @param timeout_ms 超时时间（毫秒）
 */
void audio_player_wait_idle(uint32_t timeout_ms);

/**
 * @brief 设置OPUS播放器软件增益
 * 
 * 设置当前活动的OPUS解码器的软件增益百分比
 * 仅在OPUS播放器运行时有效
 * 
 * @param gain_percent 增益百分比，100表示原音量，200表示放大2倍，50表示减半
 * @return esp_err_t ESP_OK-成功，ESP_ERR_INVALID_STATE-没有活动的OPUS播放器
 */
esp_err_t audio_player_set_opus_software_gain(int gain_percent);

/**
 * @brief 获取OPUS播放器当前软件增益设置
 * 
 * @return int 当前软件增益百分比，如果没有活动的OPUS播放器则返回100
 */
int audio_player_get_opus_software_gain(void);

#ifdef __cplusplus
}
#endif
