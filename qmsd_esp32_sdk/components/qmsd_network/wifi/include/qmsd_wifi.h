#pragma once

#include "esp_wifi_types.h"

#define QMSD_WIFI_EVENT "qmsd_wifi_event"

typedef enum {
    QMSD_WIFI_EVENT_CONNECT = WIFI_EVENT_MAX + 1,
    QMSD_WIFI_EVENT_DISCONNECT,
} qmsd_wifi_event_t;

#ifdef __cplusplus
extern "C" {
#endif

void qmsd_wifi_init();

#ifdef __cplusplus
}
#endif
