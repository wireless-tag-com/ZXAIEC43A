#ifndef __HTTP_SERVER_REQ_H__
#define __HTTP_SERVER_REQ_H__

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 请求 OTA 更新 URL
 * @param version 当前固件版本号字符串
 * @return ESP_OK 请求成功，其他值表示请求失败
 * @details 向服务器发送 OTA 请求，检查是否有新的固件版本可用，
 *          并获取下载 URL。请求成功后可通过 aiha_ota_get_url() 获取 URL
 */
esp_err_t aiha_ota_req_url(const char* version);

/**
 * @brief 获取 OTA 下载 URL
 * @return OTA 固件下载 URL 字符串指针，如果没有可用更新则返回 NULL
 * @details 获取通过 aiha_ota_req_url() 请求得到的 OTA 固件下载地址
 */
char* aiha_ota_get_url();

#ifdef __cplusplus
}
#endif

#endif
