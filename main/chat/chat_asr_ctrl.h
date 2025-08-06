#pragma once

#include "cmd_match.h"

/**
 * @brief 初始化聊天ASR控制模块
 * @note 初始化语音识别控制相关的组件和配置，为后续的语音识别功能做准备
 */
void chat_asr_ctrl_init();

/**
 * @brief 处理ASR识别结果
 * @param asr_result ASR识别到的文本结果
 * @param answer 输出参数，存储处理后的回答文本
 * @return true 如果成功处理了ASR结果，false 如果处理失败或未识别到有效指令
 * @note 根据ASR识别结果进行本地指令匹配，如音量控制、退出对话等，返回处理结果
 */
bool chat_asr_ctrl_deal_asr_result(const char* asr_result, char* answer);
