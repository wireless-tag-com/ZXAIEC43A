#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <errno.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "fs_utils";

// 使用 stat() 系统调用检查文件是否存在
bool fs_file_exists(const char* path) {
    const char* actual_path = path;
    struct stat file_stat;
    return (stat(actual_path, &file_stat) == 0);
}

int fs_delete_matching_files(const char* dir_path, const char* match_pattern, const char* exclude_filename) {
    // 参数检查
    if (!dir_path || !match_pattern) {
        ESP_LOGE(TAG, "Invalid parameters: dir_path or match_pattern is NULL");
        return -1;
    }

    // 如果exclude_filename包含路径，提取文件名部分
    const char* actual_exclude_filename = NULL;
    if (exclude_filename) {
        const char* last_slash = strrchr(exclude_filename, '/');
        if (last_slash) {
            // 找到最后一个斜杠，取其后的部分作为文件名
            actual_exclude_filename = last_slash + 1;
        } else {
            // 没有斜杠，整个字符串就是文件名
            actual_exclude_filename = exclude_filename;
        }
    }

    // 打开目录
    DIR* dir = opendir(dir_path);
    if (!dir) {
        ESP_LOGE(TAG, "Failed to open directory: %s", dir_path);
        return -1;
    }

    int deleted_count = 0;
    struct dirent* entry;

    while ((entry = readdir(dir)) != NULL) {
        // 跳过 . 和 ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // 从文件名中提取开始到@或.之间的部分
        char file_prefix[256];
        const char* at_pos = strchr(entry->d_name, '@');
        const char* dot_pos = strchr(entry->d_name, '.');
        
        // 找到最早出现的@或.位置
        const char* end_pos = NULL;
        if (at_pos && dot_pos) {
            end_pos = (at_pos < dot_pos) ? at_pos : dot_pos;
        } else if (at_pos) {
            end_pos = at_pos;
        } else if (dot_pos) {
            end_pos = dot_pos;
        }
        
        if (end_pos) {
            // 提取前缀部分
            size_t prefix_len = end_pos - entry->d_name;
            if (prefix_len >= sizeof(file_prefix)) {
                prefix_len = sizeof(file_prefix) - 1;
            }
            strncpy(file_prefix, entry->d_name, prefix_len);
            file_prefix[prefix_len] = '\0';
        } else {
            // 没有找到@或.，使用整个文件名
            strncpy(file_prefix, entry->d_name, sizeof(file_prefix) - 1);
            file_prefix[sizeof(file_prefix) - 1] = '\0';
        }

        // 检查提取的前缀是否与match_pattern匹配
        if (strcmp(file_prefix, match_pattern) == 0) {
            // 检查是否是要排除的文件名
            if (actual_exclude_filename && strcmp(entry->d_name, actual_exclude_filename) == 0) {
                ESP_LOGD(TAG, "Skipping excluded file: %s", entry->d_name);
                continue;
            }

            // 构建完整文件路径
            char full_path[512];
            int path_len = strlen(dir_path);
            // 确保目录路径以'/'结尾
            if (dir_path[path_len - 1] == '/') {
                snprintf(full_path, sizeof(full_path), "%s%s", dir_path, entry->d_name);
            } else {
                snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);
            }

            // 删除文件
            if (remove(full_path) == 0) {
                deleted_count++;
                ESP_LOGI(TAG, "Deleted file: %s (matched prefix: %s)", full_path, file_prefix);
            } else {
                ESP_LOGE(TAG, "Failed to delete file: %s", full_path);
            }
        }
    }

    closedir(dir);
    return deleted_count;
}

void fs_list_mp3_files(const char* path) {
    // 打开目录
    DIR* dir = opendir(path);
    if (dir == NULL) {
        ESP_LOGE(TAG, "Failed to open directory: %s", path);
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        // Check if the entry is a regular file
        if (entry->d_type == DT_REG) {
            // Check if the file extension is .jpg
            const char* filename = entry->d_name;
            size_t len = strlen(filename);
            if (len > 4 && strcmp(&filename[len - 4], ".mp3") == 0) {
                ESP_LOGI(TAG, "Found MP3 file: %s/%s", path, filename);
            }
        }
    }
    closedir(dir);
}

bool fs_find_file(const char* path, const char* filename, char* full_path, uint32_t full_path_size) {
    // 参数检查
    if (!path || !filename || !full_path) {
        ESP_LOGE(TAG, "Invalid parameters: path, filename or full_path is NULL");
        return 0;
    }

    // 初始化返回路径为空
    full_path[0] = '\0';

    // 打开目录
    DIR* dir = opendir(path);
    if (!dir) {
        ESP_LOGE(TAG, "Failed to open directory: %s", path);
        return 0;
    }

    struct dirent* entry;

    ESP_LOGD(TAG, "Searching for first file starting with '%s' in directory: %s", filename, path);

    while ((entry = readdir(dir)) != NULL) {
        // 跳过 . 和 ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // 检查文件名是否以filename开头
        if (strncmp(entry->d_name, filename, strlen(filename)) == 0) {
            int path_len = strlen(path);
            // 确保目录路径以'/'结尾
            if (path[path_len - 1] == '/') {
                snprintf(full_path, full_path_size, "%s%s", path, entry->d_name);
            } else {
                snprintf(full_path, full_path_size, "%s/%s", path, entry->d_name);
            }

            ESP_LOGI(TAG, "Found first matching file: %s -> %s", filename, full_path);
            closedir(dir);
            return 1;
        }
    }

    ESP_LOGI(TAG, "No files found starting with '%s' in directory: %s", filename, path);
    closedir(dir);
    return 0;
}
