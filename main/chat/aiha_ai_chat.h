#pragma once

#include "gx8006.h"
#include "aiha_websocket.h"

/**
 * @brief 处理AI聊天过程中的错误
 * @param error_code 错误代码，定义在allinone_error_code_t枚举中
 * @note 根据不同的错误代码执行相应的错误处理逻辑，如播放错误提示音、重置连接等
 */
void aiha_chat_deal_error(allinone_error_code_t error_code);

/**
 * @brief AI音频接收回调函数
 * @param status 音频状态，包含音频数据的处理状态信息
 * @param data 音频数据缓冲区指针
 * @param len 音频数据长度
 * @note 当GX8006芯片接收到音频数据时，会调用此函数进行音频数据的处理和转发
 */
void aiha_audio_recv_callback(gx8006_audio_status_t status, uint8_t* data, uint32_t len);

/**
 * @brief 启动AI聊天功能
 * @note 初始化AI聊天相关的组件，建立WebSocket连接，准备接收和处理语音数据
 */
void aiha_ai_chat_start();
