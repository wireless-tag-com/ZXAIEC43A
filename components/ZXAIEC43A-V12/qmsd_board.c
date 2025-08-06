#include "string.h"
#include "hal/gpio_hal.h"
#include "esp_log.h"
#include "qmsd_utils.h"
#include "qmsd_board.h"
#include "qmsd_lcd_wrapper.h"

#define TAG "QMSD_BOARD"

static qmsd_board_config_t g_board_config;

void qmsd_board_init(qmsd_board_config_t* config) {
    memcpy(&g_board_config, config, sizeof(qmsd_board_config_t));
    if (BOARD_RESET_PIN > -1) {
        gpio_hal_iomux_func_sel(GPIO_PIN_MUX_REG[BOARD_RESET_PIN], PIN_FUNC_GPIO);
        gpio_set_direction(BOARD_RESET_PIN, GPIO_MODE_OUTPUT);
        gpio_set_level(BOARD_RESET_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(2));
        gpio_set_level(BOARD_RESET_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(120));
    }
}
