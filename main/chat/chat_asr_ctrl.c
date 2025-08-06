#include "aiha_audio_http.h"
#include "aiha_websocket.h"
#include "audio_hardware.h"
#include "audio_player_user.h"
#include "cmd_match.h"
#include "esp_log.h"

/** @brief 问题字符串的最大长度 */
#define AI_QUEST_STRING_SIZE 128
/** @brief 回答字符串的最大长度 */
#define AI_RSP_STRING_SIZE 256

#define TAG "chat.asr_ctrl"

static bool cmd_vol_cb(const char* id, command_result_t cmd_result, char* answer, uint32_t answer_len) {
    if (cmd_result.dir == CMD_DIR_VALUE_EXACT) {
        audio_hardware_set_volume(cmd_result.value);
    } else if (cmd_result.dir == CMD_DIR_VALUE_MODIFY) {
        audio_hardware_add_volume(cmd_result.value);
    } else if (cmd_result.dir == CMD_DIR_MAX) {
        audio_hardware_set_volume(100);
    } else if (cmd_result.dir == CMD_DIR_MIN) {
        audio_hardware_set_volume(10);
    } else if (cmd_result.dir == CMD_DIR_UP) {
        audio_hardware_add_volume(10);
    } else if (cmd_result.dir == CMD_DIR_DOWN) {
        audio_hardware_add_volume(-10);
    } else {
        return false;
    }

    sprintf(answer, "音量已设置为 %d", audio_hardware_get_volume());
    return true;
}

static bool cmd_exit_cb(const char* id, command_result_t cmd_result, char* answer, uint32_t answer_len) {
    sprintf(answer, "和你聊天很开心, 下次见");
    extern void aiha_chat_deal_error(allinone_error_code_t error_code);
    aiha_chat_deal_error(ALLINONE_ERROR_CODE_USER_EXIT);
    return false;  // 不需要tts，直接使用gx8006的退出
}

bool chat_asr_ctrl_deal_asr_result(const char* asr_result, char* answer) {
    bool ret = cmd_deal_with_string(asr_result, answer, AI_RSP_STRING_SIZE);
    if (ret) {
        ESP_LOGI(TAG, "asr finish, asr_result: %s, answer: %s", asr_result, answer);
    }
    return ret;
}

void chat_asr_ctrl_init() {
    cmd_deal_register_t cmd_vol = {
        .id = "vol",
        .keyword = { "音量", "声音" },
        .keyword_num = 2,
        .dir_down_words = "响",
        .dir_up_words = "",
        .rules = CMD_RULES_VALUE,
        .cb = cmd_vol_cb,
    };
    cmd_deal_register(&cmd_vol);

    cmd_deal_register_t cmd_quit = {
        .id = "quit",
        .keyword = { "退出", "不要再说", "闭嘴", "退下" },
        .keyword_num = 4,
        .dir_down_words = "",
        .dir_up_words = "",
        .rules = CMD_RULES_PART,
        .cb = cmd_exit_cb,
    };
    cmd_deal_register(&cmd_quit);
}
