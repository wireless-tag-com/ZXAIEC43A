#include "chat_notify.h"

chat_notify_t chat_notify_list[MAX_NOTIFY_TYPE] = {
    [NOTIFY_NOT_BIND] = { 
        .notify_id = NOTIFY_NOT_BIND, .path = "device_not_bind", .enable = 1, 
        .tts_sync_type = TTS_SYNC_DISABLE, .tts_text = "设备未绑定，请使用小程序进行绑定",
    },
    [NOTIFY_WIFI_CONNECT] = {
        .notify_id = NOTIFY_WIFI_CONNECT, .path = "wifi_connect", .enable = 1, 
        .tts_sync_type = TTS_SYNC_DISABLE, .tts_text = "太棒了,WiFi 连接成功",
    },
    [NOTIFY_WIFI_FAILED] = { 
        .notify_id = NOTIFY_WIFI_FAILED, .path = "wifi_failed", .enable = 1, 
        .tts_sync_type = TTS_SYNC_DISABLE, .tts_text = "WiFi 连接失败，换个试试",
    },
    [NOTIFY_WIFI_STA_AUTH_FAILED] = { 
        .notify_id = NOTIFY_WIFI_STA_AUTH_FAILED, .path = "wifi_sta_auth_failed", .enable = 1, 
        .tts_sync_type = TTS_SYNC_DISABLE, .tts_text = "WiFi 密码好像不对，再试试呢",
    },
    [NOTIFY_WIFI_AP_NOT_FOUND] = { 
        .notify_id = NOTIFY_WIFI_AP_NOT_FOUND, .path = "wifi_ap_not_found", .enable = 1, 
        .tts_sync_type = TTS_SYNC_FROM_FILE, .tts_text = "未找到您要连接的WiFi",
    },
    [NOTIFY_WIFI_SERVER_ERROR] = { 
        .notify_id = NOTIFY_WIFI_SERVER_ERROR, .path = "wifi_server_error", .enable = 1, 
        .tts_sync_type = TTS_SYNC_FROM_FILE, .tts_text = "服务器连接失败，建议您检查下网络哦",
    },
    [NOTIFY_OTA_START] = { 
        .notify_id = NOTIFY_OTA_START, .path = "ota_start", .enable = 1, 
        .tts_sync_type = TTS_SYNC_DISABLE, .tts_text = "OTA升级开始, 我们等会聊天吧",
    },
    [NOTIFY_OTA_SUCCESS] = { 
        .notify_id = NOTIFY_OTA_SUCCESS, .path = "ota_success", .enable = 1, 
        .tts_sync_type = TTS_SYNC_DISABLE, .tts_text = "太好了，升级完成",
    },
    [NOTIFY_OTA_FAILED] = { 
        .notify_id = NOTIFY_OTA_FAILED, .path = "ota_failed", .enable = 1, 
        .tts_sync_type = TTS_SYNC_DISABLE, .tts_text = "升级遇到了一些问题，建议您重启设备后重试",
    },
    [NOTIFY_CLOSE] = { 
        .notify_id = NOTIFY_CLOSE, .path = "close", .enable = 0, 
        .tts_sync_type = TTS_SYNC_DISABLE, .tts_text = "即将关机",
    },
    [NOTIFY_LOW_BATTERY] = { 
        .notify_id = NOTIFY_LOW_BATTERY, .path = "low_battery", .enable = 0, 
        .tts_sync_type = TTS_SYNC_DISABLE, .tts_text = "电量低，请及时充电",
    },
    [NOTIFY_CHAT_FAILED] = { 
        .notify_id = NOTIFY_CHAT_FAILED, .path = "chat_failed", .enable = 1, 
        .tts_sync_type = TTS_SYNC_DISABLE, .tts_text = "网络有点不太稳定呢",
    },
    [NOTIFY_CHAT_WAKEUP] = { 
        .notify_id = NOTIFY_CHAT_WAKEUP, .path = "wakeup", .enable = 1, 
        .tts_sync_type = TTS_SYNC_FROM_NET, .tts_text = "你好呀",
    },
    [NOTIFY_CHAT_WAKEUP_VC] = { 
        .notify_id = NOTIFY_CHAT_WAKEUP_VC, .path = "wakeup1", .enable = 1, 
        .tts_sync_type = TTS_SYNC_FROM_NET, .tts_text = "在呢",
    },
    [NOTIFY_CHAT_WAKEUP_VC2] = { 
        .notify_id = NOTIFY_CHAT_WAKEUP_VC2, .path = "wakeup2", .enable = 1, 
        .tts_sync_type = TTS_SYNC_FROM_NET, .tts_text = "怎么呢",
    },
    [NOTIFY_CONNECT_SUCCESS] = { 
        .notify_id = NOTIFY_CONNECT_SUCCESS, .path = "connect_success", .enable = 1, 
        .tts_sync_type = TTS_SYNC_FROM_NET, .tts_text = "网络连接成功, 快来和我聊天吧",
    },
    [NOTIFY_CHAT_EXIT] = { 
        .notify_id = NOTIFY_CHAT_EXIT, .path = "chat_exit", .enable = 1, 
        .tts_sync_type = TTS_SYNC_FROM_NET, .tts_text = "和你聊天很开心, 下次见",
    },
    [NOTIFY_CHAT_VAD_END] = { 
        .notify_id = NOTIFY_CHAT_VAD_END, .path = "vad_end", .enable = 0, 
        .tts_sync_type = TTS_SYNC_DISABLE, .tts_text = "收音结束",
    },
    [NOTIFY_STARTUP] = { .notify_id = NOTIFY_STARTUP, .path = "open_start", .enable = 1, 
        .tts_sync_type = TTS_SYNC_FROM_FILE, .tts_text = "我是小明,很高兴认识你",
    },
};
