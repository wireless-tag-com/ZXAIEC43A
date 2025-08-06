#pragma once

#include <stdint.h>
#include "gx8006_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

// 常量定义
#define GX8006_VAD_FRAME_MS          40      // VAD帧时长，单位ms
#define GX8006_VAD_MAX_TIMEOUT_MS    (0xff * GX8006_VAD_FRAME_MS)  // VAD最大超时时间
#define GX8006_VAD_SENSITIVITY_MAX   100     // VAD灵敏度最大值
#define GX8006_MIC_GAIN_MAX          32      // 麦克风增益最大值
#define GX8006_VAD_SENSITIVITY_DEFAULT 45

typedef enum {
    GX8006_FORMAT_PCM = 0,   // only spk support
    GX8006_FORMAT_SPEEX = 1, // not support now
    GX8006_FORMAT_OPUS = 2,
    GX8006_FORMAT_MP3 = 3, // not support now
} __attribute__((packed)) gx8006_audio_format_t;

// 确保枚举大小为1字节的编译时检查
_Static_assert(sizeof(gx8006_audio_format_t) == 1, "gx8006_audio_format_t must be 1 byte");

typedef struct {
    uint32_t sample_rate;  /* 采样率（大端格式传输） */
    uint8_t bit;           /* 采样位深 */
    uint8_t channels;      /* 音频通道数量 */
    uint8_t gain;            /* 麦克风增益, 范围 0-32dB */
    gx8006_audio_format_t format;        /* 传输数据格式 */
    uint32_t max_recv_len; /* 单次可以接收的最大数据, 单位字节 */
} __attribute__((packed)) gx8006_mic_config_t ;

typedef struct {
    uint32_t sample_rate;  /* 采样率（大端格式传输） */
    uint8_t bit;           /* 采样位深 */
    uint8_t volume;        /* 音量 */
    gx8006_audio_format_t format;        /* 传输数据格式 */
    uint32_t max_send_len; /* 单次可以发送的最大数据, 单位字节 */
} __attribute__((packed)) gx8006_spk_config_t;

typedef enum {
    GX8006_AUDIO_START = 0x00,
    GX8006_AUDIO_RUNNING = 0x01,
    GX8006_AUDIO_END = 0x02,
    GX8006_AUDIO_FULL_FRAME = 0x03,
    GX8006_AUDIO_WAKEUP = 0x04,
    GX8006_AUDIO_SLEEP = 0x05,
} gx8006_audio_status_t;

typedef enum {
    // 默认使用 UART, I2S暂不支持, 0X00 UART, 0X01  I2S
    GX8006_AUDIO_CHANNEL = 0x00,

    // 默认开启状态, 0X00 麦克风关闭, 0X01 麦克风开启
    GX8006_MIC_SWITCH,

    // 默认开启状态, 0X00 扬声器关闭, 0X01 扬声器开启
    GX8006_SPK_SWITCH,

    // 默认开启状态, 0X00 关闭, 0X01 开启
    GX8006_WAKEUP_CMD_SWITCH = 0x03,

    // 默认开启状态, 使用固件默认 VAD 超时, 0x00 关闭;0x01 开启,使用固件里默认的vad timeout;大于0x01（长度1Byte）,
    // 静默帧数触发超时 1帧=40ms
    GX8006_VAD_SWITCH,

    // 0X01 以唤醒, 模组通知语音芯片已唤醒
    GX8006_WAKEUP_SWITCH = 0x05,

    // 0X01 已超时, 模组通知语音芯片已超时
    GX8006_TIMEOUT_SWITCH,

    // >0, 长度1或4Byte, 大端; 默认30s, 超时需重新唤醒设置数据需大于0, 长度1字节或4字节意味着在没有唤醒, vad为0,
    // 未播放音频超过30s后, 下一次需要唤醒才可以上报数据
    GX8006_SILENCE_TIMEOUT_SWITCH,

    // >0, 长度1或4Byte, 大端; 默认30s, 超时停止拾音设置数据需大于0, 长度1字节或4字节, 意味着最长连续上报30s数据
    GX8006_MAX_SOUND_PICKUP_TIME = 0x08,

    // 0-100, 默认45, 数值越大, 灵敏度越低
    GX8006_VAD_SENSITIVITY,

    // 0-3 （代表降噪等级）, 开启降噪, 并设置降噪等级, 默认降噪等级0, 越大降噪越明显
    // 0xFF（关闭降噪）
    GX8006_DENOISE_LEVEL,

    // 0-32, 默认20, 数值越大, 增益越大
    GX8006_MIC_GAIN_DURING_PLAY,
} gx8006_offline_voice_cfg_t;

typedef enum {
    GX8006_AUDIO_CHANNEL_UART = 0x00,
    GX8006_AUDIO_CHANNEL_I2S = 0x01,
} gx8006_audio_channel_t;

typedef enum {
    GX8006_MIC_ON = 0x01,
    GX8006_MIC_OFF = 0x00,
} gx8006_mic_onoff_t;

typedef enum {
    GX8006_SPK_ON = 0x01,
    GX8006_SPK_OFF = 0x00,
} gx8006_spk_onoff_t;

typedef enum {
    GX8006_WAKEUP_CMD_ON = 0x01,
    GX8006_WAKEUP_CMD_OFF = 0x00,
} gx8006_wakeup_cmd_onoff_t;

typedef enum {
    GX8006_VAD_ON = 0x01,
    GX8006_VAD_OFF = 0x00,
} gx8006_vad_onoff_t;

typedef enum {
    GX8006_DENOISE_LEVEL_OFF = 0xFF,
    GX8006_DENOISE_LEVEL_LOW = 0x00,
    GX8006_DENOISE_LEVEL_MEDIUM = 0x01,
    GX8006_DENOISE_LEVEL_HIGH = 0x02,
    GX8006_DENOISE_LEVEL_MAX = 0x03,
} gx8006_denoise_level_t;

typedef void (*gx8006_audio_recv_cb_t)(gx8006_audio_status_t status, uint8_t* data, uint32_t len);

/**
 * @brief 初始化GX8006音频芯片
 * @param uart_num UART端口号
 * @param tx_pin 发送引脚
 * @param rx_pin 接收引脚
 * @param flow_ctrl_pin 流控引脚，-1表示不使用流控
 * @param rst_pin 复位引脚
 * @param baudrate 波特率
 */
void gx8006_init(uint8_t uart_num, uint8_t tx_pin, uint8_t rx_pin, int8_t flow_ctrl_pin, uint8_t rst_pin, uint32_t baudrate);

/**
 * @brief 请求获取GX8006芯片版本信息
 */
void gx8006_req_version();

/**
 * @brief 配置麦克风参数
 * @param config 麦克风配置结构体指针
 */
void gx8006_config_mic(gx8006_mic_config_t* config);

/**
 * @brief 配置扬声器参数
 * @param config 扬声器配置结构体指针
 */
void gx8006_config_spk(gx8006_spk_config_t* config);

/**
 * @brief 设置扬声器音量
 * @param volume 音量值
 */
void gx8006_set_volume(uint8_t volume);

/**
 * @brief 获取流控写使能状态
 * @return 写使能状态，1为可写，0为不可写
 */
uint8_t gx8006_flow_get_write_enable();

/**
 * @brief 开始音频写入
 */
void gx8006_audio_write_start();

/**
 * @brief 停止音频写入
 */
void gx8006_audio_write_stop();

/**
 * @brief 写入音频数据
 * @param data 音频数据缓冲区指针
 * @param len 数据长度
 */
void gx8006_write_audio_data(uint8_t* data, uint32_t len);

/**
 * @brief 设置音频接收回调函数
 * @param callback 回调函数指针
 */
void gx8006_set_audio_recv_callback(gx8006_audio_recv_cb_t callback);

/**
 * @brief 设置录音降噪状态
 * @param record_mode 录音模式
 * @param denoise_level 降噪等级
 */
void gx8006_set_record_denoise_status(int8_t record_mode, int8_t denoise_level);

/**
 * @brief 通知播放状态
 * @param status 播放状态
 */
void gx8006_notify_play_status(uint8_t status);

/**
 * @brief 录音播放测试
 * @param record_sec 录音时长，单位秒
 */
void gx8006_record_and_play_test(uint8_t record_sec);

/**
 * @brief 离线语音配置
 * @param cfg_cmd 配置命令
 * @param cfg_data 配置数据
 */
void gx8006_offline_voice_cfg(gx8006_offline_voice_cfg_t cfg_cmd, uint32_t cfg_data);

/**
 * @brief 配置VAD（语音活动检测）开关
 * @param vad_onoff VAD开关状态
 */
void gx8006_vad_onoff_cfg(gx8006_vad_onoff_t vad_onoff);

/**
 * @brief 配置VAD超时时间
 * @param timeout_ms 超时时间，单位毫秒
 */
void gx8006_vad_timeout_cfg(uint16_t timeout_ms);

/**
 * @brief 配置音频通道类型
 * @param channel_type 通道类型（UART或I2S）
 */
void gx8006_audio_channel_cfg(gx8006_audio_channel_t channel_type);

/**
 * @brief 配置麦克风开关
 * @param mic_onoff 麦克风开关状态
 */
void gx8006_mic_onoff_cfg(gx8006_mic_onoff_t mic_onoff);

/**
 * @brief 配置扬声器开关
 * @param spk_onoff 扬声器开关状态
 */
void gx8006_spk_onoff_cfg(gx8006_spk_onoff_t spk_onoff);

/**
 * @brief 配置唤醒命令开关
 * @param wakeup_cmd_onoff 唤醒命令开关状态
 */
void gx8006_wakeup_cmd_onoff_cfg(gx8006_wakeup_cmd_onoff_t wakeup_cmd_onoff);

/**
 * @brief 进入唤醒模式
 */
void gx8006_into_wakeup_mode(void);

/**
 * @brief 进入睡眠模式
 */
void gx8006_into_sleep_mode(void);

/**
 * @brief 配置睡眠超时时间
 * @param sleep_timeout_sec 睡眠超时时间，单位秒
 */
void gx8006_sleep_timeout_cfg(uint32_t sleep_timeout_sec);

/**
 * @brief 配置最大拾音时间
 * @param max_sound_pickup_sec 最大拾音时间，单位秒
 */
void gx8006_max_sound_pickup_time_cfg(uint32_t max_sound_pickup_sec);

/**
 * @brief 配置VAD灵敏度
 * @param sensitivity 灵敏度值（0-100，数值越大灵敏度越低）
 */
void gx8006_vad_sensitivity_cfg(uint8_t sensitivity);

/**
 * @brief 配置降噪等级
 * @param denoise_level 降噪等级
 */
void gx8006_denoise_level_cfg(gx8006_denoise_level_t denoise_level);

/**
 * @brief 配置播放期间麦克风增益
 * @param gain 增益值，范围0-32，默认20，数值越大增益越大
 */
void gx8006_mic_gain_during_play_cfg(uint8_t gain);

/**
 * @brief 等待芯片启动完成
 * @param timeout_ms 超时时间，单位毫秒
 * @return true 启动成功，false 启动超时
 */
bool gx8006_wait_startup(uint32_t timeout_ms);

/**
 * @brief 获取唤醒保持时间
 * @return 唤醒保持时间，单位毫秒
 */
uint32_t gx8006_get_wakeup_keep_ms();


/**
 * @brief 退出聊天模式
 * @param is_exit true:需要退出聊天，休眠时间改为1s，false:确定休眠后，休眠时间改为30s
 */
void gx8006_exit_chat_mode(bool is_exit);

/**
 * @brief 获取是否在唤醒模式
 * @return true 在唤醒模式，false 不在唤醒模式
 */
bool gx8006_in_wakeup();

#ifdef __cplusplus
}
#endif