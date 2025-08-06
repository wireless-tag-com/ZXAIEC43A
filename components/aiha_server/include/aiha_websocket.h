#pragma once

#include "cJSON.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/ringbuf.h"
#include "freertos/task.h"
#include "stdint.h"

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief AIHA 系统状态枚举
 * @details 定义了 AIHA 系统在整个对话流程中的各种状态标志位
 */
typedef enum {
    ALLINONE_STATUS_NOT_START = BIT(5),          /**< 系统未启动 */
    ALLINONE_STATUS_RUNNING = BIT(6),            /**< 系统运行中 */
    ALLINONE_STATUS_AUDIO_START = BIT(7),        /**< 音频采集开始 */
    ALLINONE_STATUS_AUDIO_FINISH = BIT(8),       /**< 音频采集完成 */
    ALLINONE_STATUS_ASR_VALID = BIT(9),          /**< 语音识别结果有效 */
    ALLINONE_STATUS_ASR_FINISH = BIT(10),        /**< 语音识别完成 */
    ALLINONE_STATUS_ANSWER_TEXT_VALID = BIT(11), /**< 回答文本有效 */
    ALLINONE_STATUS_ANSWER_MP3_VALID = BIT(12),  /**< 回答 MP3 有效 */
    ALLINONE_STATUS_TTS_FINISH = BIT(13),        /**< 语音合成完成 */
    ALLINONE_STATUS_FINISH = BIT(15),            /**< 请求完成 */
    ALLINONE_STATUS_ERROR = BIT(16),             /**< 错误 */
    ALLINONE_STATUS_USER_EXIT = BIT(17),         /**< 用户主动退出对话 */
} allinone_status_t;

/**
 * @brief AIHA 错误代码枚举
 * @details 定义了系统运行过程中可能出现的各种错误类型
 */
typedef enum {
    ALLINONE_ERROR_CODE_NONE = 0,                /**< 无错误 */
    ALLINONE_ERROR_CODE_HTTP_ERROR = -1,         /**< HTTP 请求错误，服务器出小差了，请重试 */
    ALLINONE_ERROR_CODE_AUDIO_WRITE_ERROR = -2,  /**< 音频写入错误，服务器出小差了，请重试 */
    ALLINONE_ERROR_CODE_ASR_ERROR = -3,          /**< 语音识别错误，没听懂你在说什么 */
    ALLINONE_ERROR_CODE_TTS_ERROR = -4,          /**< 语音合成错误，服务器出小差了，请重试 */
    ALLINONE_ERROR_CODE_USER_EXIT = -5,          /**< 用户主动退出 */
    ALLINONE_ERROR_CODE_NO_MONEY = -6,           /**< 可用对话次数不足，请充值/未知错误 */
    ALLINONE_ERROR_CODE_MAX,                     /**< 错误代码最大值 */
} allinone_error_code_t;

/**
 * @brief 音频状态枚举
 * @details 定义了音频处理过程中的不同状态
 */
typedef enum {
    ALLINONE_AUDIO_STATUS_START = 0,      /**< 音频开始 */
    ALLINONE_AUDIO_STATUS_PROCESSING = 1, /**< 音频处理中 */
    ALLINONE_AUDIO_STATUS_END = 2,        /**< 音频结束 */
} allinone_audio_status_t;

/**
 * @brief WebSocket 事件位掩码
 * @details 用于事件组的所有事件位掩码
 */
#define ALL_WEBSOCKET_EVENT_BITS (0x1ffff)

/**
 * @brief WebSocket 错误回调函数类型
 * @param error_code 错误代码
 */
typedef void (*aiha_websocket_error_cb_t)(allinone_error_code_t error_code);

/**
 * @brief ASR 完成回调函数类型
 * @param asr_result 语音识别结果
 * @param answer 服务器回答
 * @return 是否继续处理
 */
typedef bool (*aiha_request_asr_finish_cb_t)(const char* asr_result, const char* answer);

/**
 * @brief WebSocket 音频接收回调函数类型
 * @param data 音频数据指针
 * @param size 音频数据大小
 * @param status 音频状态
 */
typedef void (*aiha_websocket_audio_recv_cb_t)(const uint8_t* data, uint32_t size, allinone_audio_status_t status);

/**
 * @brief WebSocket 文本接收回调函数类型
 * @param asr_result ASR 识别结果
 * @param asr_finish ASR 是否完成 (0: 未完成, 1: 完成)
 * @param answer 服务器回答文本
 */
typedef void (*aiha_websocket_text_recv_cb_t)(char* asr_result, uint8_t asr_finish, char* answer);

/**
 * @brief AIHA WebSocket 处理句柄结构体
 * @details 包含 WebSocket 连接所需的配置信息和回调函数
 */
typedef struct {
    char production_id[32];                           /**< 产品ID */
    char audio_upload_format[16];                     /**< 音频上传格式 */
    char audio_spk_format[16];                        /**< 音频播放格式 */
    aiha_websocket_audio_recv_cb_t audio_recv_cb;     /**< 音频接收回调函数 */
    aiha_websocket_text_recv_cb_t text_recv_cb;       /**< 文本接收回调函数, 未实现 */
    aiha_websocket_error_cb_t error_cb;               /**< 错误回调函数 */
    aiha_request_asr_finish_cb_t asr_finish_cb;       /**< ASR 完成回调函数 */
} aiha_websocket_handle_t;

/**
 * @brief 初始化 AIHA WebSocket 客户端
 * @param handle WebSocket 处理句柄指针
 * @details 使用提供的配置信息初始化 WebSocket 客户端，设置回调函数
 */
void aiha_websocket_init(aiha_websocket_handle_t* handle);

/**
 * @brief 连接到 AIHA WebSocket 服务器
 * @details 建立与服务器的 WebSocket 连接，开始对话会话
 */
void aiha_websocket_connect();

/**
 * @brief 开始音频上传
 * @details 向服务器发送音频上传开始信号，准备开始传输音频数据
 */
void aiha_websocket_audio_upload_start();

/**
 * @brief 结束音频上传
 * @details 向服务器发送音频上传结束信号，表示音频数据传输完成
 */
void aiha_websocket_audio_upload_end();

/**
 * @brief 上传音频数据
 * @param data 音频数据指针
 * @param size 音频数据大小
 * @details 将音频数据块发送到服务器进行处理
 */
void aiha_websocket_audio_upload_data(uint8_t* data, uint32_t size);

/**
 * @brief 获取当前系统状态
 * @return 当前的系统状态标志位组合
 * @details 返回当前 AIHA 系统的运行状态，可以通过位运算检查具体状态
 */
allinone_status_t aiha_websocket_get_status();

/**
 * @brief 获取错误代码
 * @return 当前的错误代码
 * @details 当系统出现错误时，返回具体的错误类型代码
 */
allinone_error_code_t aiha_websocket_get_error_code();

/**
 * @brief 调试输出系统状态
 * @details 通过日志输出当前系统的状态信息，用于调试
 */
void aiha_websocket_debug_status();

/**
 * @brief 异步停止所有请求
 * @details 停止当前正在进行的所有对话请求和音频处理
 */
void aiha_websocket_req_stop_all_async();

/**
 * @brief 检查 WebSocket 连接状态
 * @return true 已连接，false 未连接
 * @details 检查与服务器的 WebSocket 连接是否正常
 */
bool aiha_websocket_is_connected();

/**
 * @brief 设置唤醒状态
 * @details 设置系统为唤醒状态，准备接收用户输入
 */
void aiha_websocket_set_wakeup();

/**
 * @brief 获取音乐播放状态
 * @return true 正在播放音乐，false 未播放音乐
 * @details 检查当前是否有音乐在播放
 */
bool aiha_websocket_is_music_playing();

/**
 * @brief 设置音乐播放状态
 * @param playing true 开始播放音乐，false 停止播放音乐
 * @details 控制音乐的播放和停止状态
 */
void aiha_websocket_set_music_playing(bool playing);

/**
 * @brief 获取语音合成哈希码
 * @return 语音合成哈希码
 * @details 获取当前语音合成任务的唯一标识哈希码
 */
int aiha_websocket_get_tts_hashcode(void);

/**
 * @brief 获取语音合成音量
 * @return 语音合成音量 (单位: dB)
 * @details 获取当前语音合成的音量大小，以分贝为单位
 */
double aiha_websocket_get_tts_volume_db(void);

#ifdef __cplusplus
}
#endif
