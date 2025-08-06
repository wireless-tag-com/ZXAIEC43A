#pragma once

#include "esp_http_client.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief AI 任务内存分配配置
 * @details 0: 不使用 PSRAM，1: 使用 PSRAM
 *          控制 AI 相关任务是否在 PSRAM 中分配内存
 */
#define AI_TASK_IN_PSRAM 0

/**
 * @brief 设置产品ID
 * @param production_id 要设置的产品ID字符串
 * @details 设置当前设备的产品标识符，用于服务器识别设备类型
 */
void aiha_http_set_production_id(const char* production_id);

/**
 * @brief 获取产品ID
 * @return 当前设置的产品ID字符串指针
 * @details 返回之前通过 aiha_http_set_production_id 设置的产品ID
 */
const char* aiha_get_production_id();

/**
 * @brief 获取设备ID
 * @return 设备唯一标识符字符串指针
 * @details 获取当前设备的唯一标识符，通常基于设备硬件信息生成
 */
char* aiha_get_device_id();

/**
 * @brief 为 HTTP 请求添加签名
 * @param client HTTP 客户端句柄
 * @details 为 HTTP 请求添加必要的认证签名信息，确保请求的安全性
 */
void aiha_request_add_sign(esp_http_client_handle_t client);

/**
 * @brief 从 HTTP 事件同步系统时间
 * @param evt HTTP 客户端事件指针
 * @details 从 HTTP 响应头中获取服务器时间并同步到本地系统时间
 */
void http_sync_time_from_event(esp_http_client_event_t* evt);

#ifdef __cplusplus
}
#endif
