#pragma once

#include "stdio.h"
#include "stdbool.h"
#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef enum {
    CMD_MATCH_NONE,          // 未匹配到
    CMD_MATCH_NORMAL,        // 只匹配到文字
    CMD_MATCH_VALUE_OR_DIR,  // 匹配到文字和值
} command_match_t;

typedef enum {
    CMD_RULES_EXACT,  // 完全匹配
    CMD_RULES_PART,   // 部分匹配
    CMD_RULES_VALUE,  // 匹配值提取
} command_match_rules_t;

// 枚举调整方向
typedef enum {
    CMD_DIR_NONE,          // 未匹配到
    CMD_DIR_UP,            // 调大
    CMD_DIR_DOWN,          // 调小
    CMD_DIR_MAX,           // 调到最大
    CMD_DIR_MIN,           // 调到最小
    CMD_DIR_VALUE_EXACT,   // 确切数值
    CMD_DIR_VALUE_MODIFY,  // 修改数值
} command_dir_t;

typedef struct {
    command_match_t result;
    float value;  //
    command_dir_t dir;
} command_result_t;

typedef bool (*cmd_match_callback_t)(const char* id, command_result_t result, char* answer, uint32_t answer_len);

// 用于注册命令处理函数
typedef struct {
    const char* id;           // 用于标识命令，在callback中区分
    const char* keyword[10];  // 指向内容不能被释放
    uint8_t keyword_num;
    const char* dir_up_words;    // 指向内容不能被释放
    const char* dir_down_words;  // 指向内容不能被释放
    command_match_rules_t rules;
    cmd_match_callback_t cb;
} cmd_deal_register_t;

typedef struct cmd_deal_register_node_t {
    cmd_deal_register_t reg;
    struct cmd_deal_register_node_t* next;
} cmd_deal_register_node_t;

command_result_t cmd_deal(const char* text_in, const char* keyword, const char* dir_up, const char* dir_down, command_match_rules_t rules);

void cmd_deal_register(cmd_deal_register_t* cmd_reg);

bool cmd_deal_with_string(const char* text_in, char* answer, uint32_t answer_len);

#ifdef __cplusplus
}
#endif
