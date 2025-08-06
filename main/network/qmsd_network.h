#pragma once

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief 网络连接成功回调函数类型
 * @note 当Wi-Fi连接成功时会被调用的回调函数
 */
typedef void (*qmsd_network_connect_success_cb_t)(void);

/**
 * @brief 获取当前网络连接状态
 * @return true 如果网络已连接，false 如果网络未连接
 * @note 检查设备是否已成功连接到Wi-Fi网络
 */
bool qmsd_network_get_connected();

/**
 * @brief 启动网络连接功能
 * @param cb 网络连接成功后的回调函数
 * @note 初始化Wi-Fi连接，支持AP和STA模式，连接成功后会调用指定的回调函数
 */
void qmsd_network_start(qmsd_network_connect_success_cb_t cb);