set(requires gx8006_protocol esp_https_ota app_update qmsd_network qmsd_button storage_nvs audio_player aiha_server ZXAIEC43A-V12 ws2812)

set(src_dir . network  chat_notify chat)

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/debug)
    list(APPEND src_dir debug)
endif()

idf_component_register(
    SRC_DIRS ${src_dir}
    INCLUDE_DIRS ${src_dir}
    REQUIRES ${requires}
)

littlefs_create_partition_image(res tone_res FLASH_IN_PROJECT)