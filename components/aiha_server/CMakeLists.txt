set(requires json esp_ringbuf esp_http_client qmsd_utils qmsd_network espressif__esp_websocket_client)

idf_component_register(
	INCLUDE_DIRS include
)

add_prebuilt_library(lib${COMPONENT_NAME} lib${COMPONENT_NAME}.a REQUIRES ${requires})
target_link_libraries(${COMPONENT_LIB} INTERFACE lib${COMPONENT_NAME})
