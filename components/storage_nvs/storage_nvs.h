#pragma once

#include "esp_err.h"
#include "stdio.h"


void storage_nvs_init();

esp_err_t storage_nvs_write_blob(const char* key, const void* value, uint32_t len);

esp_err_t storage_nvs_write_str(const char* key, const char* str);

esp_err_t storage_nvs_read_str(const char* key, char** string, uint32_t* len);

esp_err_t storage_nvs_read_blob(const char* key, void** value, uint32_t* len);

esp_err_t storage_nvs_erase_key(const char* key);

esp_err_t storage_nvs_reset();