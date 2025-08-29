#pragma once

#include "stdint.h"
#include "sdkconfig.h"
#include "qmsd_board_pin.h"
#include "qmsd_board_def.h"
#include "qmsd_board_config.h"
#include "qmsd_board_utils.h"
#include "qmsd_touch.h"
#include "qmsd_gui.h"
#include "screen_driver.h"

#ifdef __cplusplus
extern "C" {
#endif

// Must ahead other init fun
void qmsd_board_init(qmsd_board_config_t* config);

#ifdef __cplusplus
}
#endif
