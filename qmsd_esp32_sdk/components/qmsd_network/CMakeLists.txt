if(CONFIG_BT_NIMBLE_BLUFI_ENABLE)
set(ext_src_dirs provisioning/blufi)
endif()

set(requires json mbedtls wifi_provisioning bt nvs_flash qmsd_utils)

if (CONFIG_IDF_TARGET_ESP32P4)
else()
    set(src_dirs wifi provisioning/smartconfig provisioning/softap ${ext_src_dirs})
endif()

idf_component_register(
	SRC_DIRS ${src_dirs}
    INCLUDE_DIRS wifi/include provisioning/include 
    REQUIRES ${requires}
)
