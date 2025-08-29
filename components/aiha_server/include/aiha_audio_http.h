#pragma once

#include <stdbool.h>
#include "esp_err.h"
#include "stdint.h"

#include "aiha_http_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief TTS (文本转语音) 回调函数类型
 * @param url TTS 音频文件的下载 URL
 * @param user_data 用户自定义数据指针
 * @details 当 TTS 请求完成时调用此回调函数，提供生成的音频文件 URL
 */
typedef void (*aiha_tts_cb_t)(const char* url, void* user_data);

/**
 * @brief 请求 TTS (文本转语音) 服务
 * @param text 要转换为语音的文本内容
 * @param cb TTS 完成回调函数，如果为 NULL 则使用全局回调函数
 * @param url 输出参数，用于存储返回的音频文件 URL
 * @param user_data 传递给回调函数的用户自定义数据
 * @return true 请求成功发送，false 请求发送失败
 * @details 向服务器发送文本转语音请求，异步处理并通过回调函数返回结果
 */
bool aiha_request_tts(const char* text, aiha_tts_cb_t cb, char* url, void* user_data);

/**
 * @brief 异步请求 TTS (文本转语音) 服务
 * @param text 要转换为语音的文本内容
 * @details 向服务器发送文本转语音请求，异步处理并通过回调函数返回结果
 */
void aiha_request_tts_async(const char* text);

/**
 * @brief 设置全局 TTS 回调函数
 * @param cb 要设置的全局回调函数
 * @details 设置全局的 TTS 回调函数，当 aiha_request_tts 的 cb 参数为 NULL 时使用
 */
void aiha_request_tts_set_cb(aiha_tts_cb_t cb);

/**
 * @brief 下载 TTS 文本到文件
 * @note 使用阻塞下载
 * @param text 要转换为语音的文本内容
 * @param filepath 要保存的文件路径
 * @return 错误码
 */
esp_err_t aiha_tts_download_audio_to_file(const char* text, const char* filepath);
    
#ifdef __cplusplus
}
#endif
