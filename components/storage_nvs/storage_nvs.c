#include "storage_nvs.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "esp_heap_caps.h"

#define TAG "NVS"

#define STORAGE_NAMESPACE "storage"

static nvs_handle_t storage_handle;

void storage_nvs_init() {
    esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &storage_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS Init Failed");
    }
}

esp_err_t storage_nvs_write_blob(const char* key, const void* value, uint32_t len) {
    if (!storage_handle) {
        return ESP_FAIL;
    }
    nvs_set_blob(storage_handle, key, value, len);
    return nvs_commit(storage_handle);
}

esp_err_t storage_nvs_write_str(const char* key, const char* str) {
    if (!storage_handle) {
        return ESP_FAIL;
    }
    nvs_set_str(storage_handle, key, str);
    return nvs_commit(storage_handle);
}

esp_err_t storage_nvs_read_str(const char* key, char** string, uint32_t* len) {
    if (!storage_handle) {
        return ESP_FAIL;
    }
    size_t required_size = 0; // value will default to 0, if not set yet in NVS
    esp_err_t err = nvs_get_str(storage_handle, key, NULL, &required_size);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
        return err;
    char* string_data = heap_caps_calloc(1, required_size + sizeof(char), MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);

    if (required_size > 0) {
        err = nvs_get_str(storage_handle, key, string_data, &required_size);
        if (err != ESP_OK) {
            free(string_data);
            return err;
        }
    }
    *len = required_size;
    *string = string_data;
    return ESP_OK;
}

esp_err_t storage_nvs_read_blob(const char* key, void** value, uint32_t* len) {
    if (!storage_handle) {
        return ESP_FAIL;
    }

    size_t required_size = 0;
    esp_err_t err = nvs_get_blob(storage_handle, key, NULL, &required_size);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        return err;
    }

    if (len) {
        if (*len && *len != required_size) {
            return ESP_FAIL;
        }
        *len = required_size;
    }

    if (!value || required_size == 0) {
        return ESP_OK;
    }

    void* run_time = *value ? *value : heap_caps_malloc_prefer(required_size, 2, MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
    if (!run_time) {
        return ESP_ERR_NO_MEM;
    }

    err = nvs_get_blob(storage_handle, key, run_time, &required_size);
    if (err != ESP_OK) {
        if (*value == NULL) {
            free(run_time);
        }
        return err;
    }

    *value = run_time;
    return ESP_OK;
}

esp_err_t storage_nvs_reset() {
    // nvs_erase_key(storage_handle, "wifiCfg");
    // nvs_erase_key(storage_handle, "nikeName");
    nvs_erase_all(storage_handle);
    return nvs_commit(storage_handle);
}

esp_err_t storage_nvs_erase_key(const char* key) {
    nvs_erase_key(storage_handle, key);
    return nvs_commit(storage_handle);
}
