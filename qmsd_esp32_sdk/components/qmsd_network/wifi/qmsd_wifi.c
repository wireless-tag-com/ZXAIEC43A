#include <esp_wifi.h>

static bool g_wifi_init = false;

void qmsd_wifi_init() {
    if (g_wifi_init) {
        return ;
    }
    g_wifi_init = true;
    esp_netif_init();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
}
