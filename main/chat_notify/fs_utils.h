#pragma once

#include "stdbool.h"
#include "string.h"

/**
 * @brief 检查文件是否存在
 * @param path 文件路径
 * @return true 如果文件存在，false 如果文件不存在
 * @note 检查指定路径的文件是否存在，用于验证音频文件等资源文件的有效性
 */
bool fs_file_exists(const char* path);

/**
 * @brief 删除匹配模式的文件
 * @param dir_path 目录路径
 * @param match_pattern 匹配模式字符串
 * @param exclude_filename 要排除的文件名
 * @return 删除的文件数量
 * @note 在指定目录中删除符合匹配模式的文件，但排除指定的文件名，用于清理临时文件
 */
int fs_delete_matching_files(const char* dir_path, const char* match_pattern, const char* exclude_filename);

/**
 * @brief 列出指定路径下的MP3文件
 * @param path 要搜索的路径
 * @note 遍历指定路径，列出所有MP3格式的音频文件，用于调试和文件管理
 */
void fs_list_mp3_files(const char* path);

/**
 * @brief 在指定路径中查找文件
 * @param path 搜索路径
 * @param filename 要查找的文件名
 * @param full_path 输出参数，存储找到文件的完整路径
 * @param full_path_size full_path缓冲区的大小
 * @return true 如果找到文件，false 如果未找到
 * @note 在指定路径及其子目录中递归查找指定文件，返回完整路径
 */
bool fs_find_file(const char* path, const char* filename, char* full_path, uint32_t full_path_size);
