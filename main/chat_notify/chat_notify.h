#pragma once

#include "esp_err.h"
#include "stdint.h"

/**
 * @brief 聊天通知状态枚举
 * @note 定义了系统中各种需要播放提示音的状态类型
 */
typedef enum {
    NOTIFY_NOT_BIND,              // 设备没有绑定
    NOTIFY_WIFI_CONNECT,          // 配网成功
    NOTIFY_WIFI_FAILED,           // WIFI连接失败
    NOTIFY_WIFI_STA_AUTH_FAILED,  // 配网密码错误
    NOTIFY_WIFI_AP_NOT_FOUND,     // 配网没有找到wifi
    NOTIFY_WIFI_SERVER_ERROR,     // 设备未链接服务器

    NOTIFY_OTA_START,    // OTA 升级开始
    NOTIFY_OTA_SUCCESS,  // OTA 升级成功
    NOTIFY_OTA_FAILED,   // OTA 升级失败

    NOTIFY_CLOSE,        // 关机提示
    NOTIFY_LOW_BATTERY,  // 低电量提示音

    NOTIFY_CHAT_VAD_END,     // 收音结束
    NOTIFY_CHAT_FAILED,      // 对话过程中的网络错误
    NOTIFY_CHAT_WAKEUP,      // 唤醒
    NOTIFY_CHAT_WAKEUP_VC,   // 对话过程中的唤醒
    NOTIFY_CHAT_WAKEUP_VC2,  // 对话过程中的唤醒

    NOTIFY_STARTUP,       // 开机提示音
    NOTIFY_CONNECT_SUCCESS,  // 服务器连接成功提示
    NOTIFY_CHAT_EXIT,        // 聊天结束

    NOTIFY_MATCH_BAT_QUERY,      // 电池查询
    NOTIFY_MATCH_FULLY_CHARGED,  // 电量满格提示
    NOTIFY_MATCH_VOL_SET,        // 音量设置

    NOTIFY_CHAT_NOT_UNDERSTAND,  // 抱歉没有

    MAX_NOTIFY_TYPE,
} chat_notify_status_t;

/**
 * @brief TTS同步类型枚举
 * @note 定义了TTS文本同步的不同方式
 */
typedef enum {
    TTS_SYNC_DISABLE = 0,     // 禁用TTS同步
    TTS_SYNC_FROM_NET = 1,    // 从网络获取TTS文本
    TTS_SYNC_FROM_FILE = 2,   // 从文件获取TTS文本
} tts_sync_type_t;

/**
 * @brief 聊天通知配置结构体
 * @note 定义了每个通知项的配置信息，包括音频文件路径、TTS文本等
 */
typedef struct {
    chat_notify_status_t notify_id;    // 通知ID
    char* path;                        // 音频文件路径
    char* tts_text;                    // TTS文本内容
    char* path_temp;                   // 临时文件路径, 只有tts_sync_type为TTS_SYNC_FROM_FILE时有效
    uint8_t enable;                    // 是否启用该通知
    tts_sync_type_t tts_sync_type;     // TTS同步类型
    uint8_t file_exist;                // 文件是否存在标志
} chat_notify_t;

/**
 * @brief 初始化聊天通知模块
 * @note 初始化通知系统，加载音频文件配置，准备播放各种提示音
 */
void chat_notify_init(void);

/**
 * @brief 播放指定状态的通知音频
 * @param notify_status 要播放的通知状态
 * @param data 附加数据指针，可为NULL
 * @note 根据通知状态播放对应的音频文件，支持TTS文本同步播放
 */
void chat_notify_audio_play(chat_notify_status_t notify_status, void* data);
