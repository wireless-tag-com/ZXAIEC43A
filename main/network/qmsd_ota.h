#pragma once

#include "esp_err.h"

/**
 * @brief 获取OTA升级状态信息
 * @param percent 输出参数，存储升级进度百分比（0-100）
 * @param status 输出参数，存储升级状态码
 * @note 获取当前OTA升级的进度和状态信息，用于显示升级进度
 */
void qmsd_ota_get_status(uint8_t* percent, uint8_t* status);

/**
 * @brief 通过HTTP检查OTA升级
 * @note 主动检查是否有新的固件版本可供升级，如果有新版本则自动开始升级流程
 */
void qmsd_check_ota_by_http();
