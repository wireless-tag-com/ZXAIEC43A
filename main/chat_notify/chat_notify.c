#include <pthread.h>
#include "esp_log.h"

#include "aiha_audio_http.h"
#include "aiha_websocket.h"
#include "audio_player_user.h"
#include "chat_notify.h"
#include "fs_utils.h"
#include "qmsd_utils.h"

#define AUDIO_PATH "/littlefs"
#define TEMP_FILE_PATH "/littlefs/temp.mp3"

#define NOTIFY_PATH_COVER(notify_path, hashcode, file_path)                    \
    do {                                                                       \
        sprintf(file_path, "%s/%s@%d.mp3", AUDIO_PATH, notify_path, hashcode); \
    } while (0)

// 声明 aiha_tts_cb 函数
extern void aiha_tts_cb(const char* url, void* user_data);
extern chat_notify_t chat_notify_list[];

#define TAG "chat_notify"

void chat_notify_audio_play(chat_notify_status_t notify_status, void* data) {
    chat_notify_t notify_item = chat_notify_list[notify_status];

    char path[256] = { 0 };

    if (notify_item.enable == 0) {
        return;
    }

    // 文件存在
    if (notify_item.file_exist == 1) {
        char path_temp[256] = { 0 };
        NOTIFY_PATH_COVER(notify_item.path, aiha_websocket_get_tts_hashcode(), path_temp);
        sprintf(path, MP3_URL_FROM_FILE"%s", path_temp);
        audio_player_play_url(path, 1);
        return;
    }
    if (notify_item.tts_sync_type == TTS_SYNC_DISABLE) {
        sprintf(path, MP3_URL_FROM_FILE"/littlefs/%s.mp3", notify_item.path);
        audio_player_play_url(path, 1);
        return;
    }

    // 网络优先
    if (notify_item.tts_sync_type == TTS_SYNC_FROM_NET) {
        aiha_request_tts_async(notify_item.tts_text);
        return;
    }

    if (notify_item.tts_sync_type == TTS_SYNC_FROM_FILE) {
        if (notify_item.path_temp) {
            sprintf(path, MP3_URL_FROM_FILE"%s", notify_item.path_temp);
            audio_player_play_url(path, 1);
            ESP_LOGI(TAG, "play tts file: %s", path);
        } else {
            ESP_LOGE(TAG, "%s tts_sync_type is TTS_SYNC_FROM_FILE, but path_temp is NULL", notify_item.path);
        }
        return;
    }
}

// 文件同步任务, 用于将tts文件同步到littlefs
// 1. 如果tts_sync_type为TTS_SYNC_FROM_FILE, 需要先读取老文件地址
// 2. 如果判断为TTS_NET, 在没文件的时候，从网络进行tts下载
// 3. 如果判断为TTS_DISABLE, 则不进行同步
static void chat_file_sync_task(void* arg) {
    chat_notify_t* notify_list = chat_notify_list;
    char file_path[256] = { 0 };
    int hashcode = 0;
    ESP_LOGI(TAG, "chat file sync task start, wait tts hashcode");
    for (int i = 0; i < MAX_NOTIFY_TYPE; i++) {
        if (notify_list[i].enable == 0) {
            continue;
        }
        if (notify_list[i].tts_sync_type == TTS_SYNC_FROM_FILE) {
            char full_path[256] = { 0 };
            if (fs_find_file(AUDIO_PATH, notify_list[i].path, full_path, sizeof(full_path))) {
                notify_list[i].path_temp = qmsd_malloc(strlen(full_path) + 1);
                strcpy(notify_list[i].path_temp, full_path);
            }
            continue;
        }
    }

    for (;;) {
        if (aiha_websocket_is_connected()) {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    for (;;) {
        hashcode = aiha_websocket_get_tts_hashcode();
        if (hashcode != 0) {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    ESP_LOGI(TAG, "tts hashcode: %d", hashcode);
    for (int i = 0; i < MAX_NOTIFY_TYPE; i++) {
        if (notify_list[i].enable == 0) {
            continue;
        }
        if (notify_list[i].tts_sync_type == TTS_SYNC_DISABLE) {
            notify_list[i].file_exist = 1;
            continue;
        }
        NOTIFY_PATH_COVER(notify_list[i].path, hashcode, file_path);
        notify_list[i].file_exist = fs_file_exists(file_path);
        if (notify_list[i].file_exist == 1) {
            fs_delete_matching_files(AUDIO_PATH, notify_list[i].path, file_path);
        }
    }

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(100));

        // check network
        for (int i = 0; i < MAX_NOTIFY_TYPE; i++) {
            if (notify_list[i].enable == 0) {
                continue;
            }

            if (notify_list[i].file_exist == 0) {
                NOTIFY_PATH_COVER(notify_list[i].path, hashcode, file_path);

                if (aiha_tts_download_audio_to_file(notify_list[i].tts_text, TEMP_FILE_PATH) != ESP_OK) {
                    vTaskDelay(pdMS_TO_TICKS(5000));
                    continue;
                }
                rename(TEMP_FILE_PATH, file_path);
                notify_list[i].file_exist = 1;
                if (notify_list[i].path_temp) {
                    free(notify_list[i].path_temp);
                    notify_list[i].path_temp = NULL;
                }
                ESP_LOGI(TAG, "success update tts file: %s", file_path);

                // 删除旧文件
                fs_delete_matching_files(AUDIO_PATH, notify_list[i].path, file_path);
            }
        }
    }
}

void chat_notify_init(void) {
    qmsd_thread_create(chat_file_sync_task, "chat_file_sync_task", 5120, NULL, 5, NULL, 0, 1);
}
