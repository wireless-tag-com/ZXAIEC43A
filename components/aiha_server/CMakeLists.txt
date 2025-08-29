set(requires json esp_ringbuf esp_http_client qmsd_utils qmsd_network esp_websocket_client)

idf_component_register(
	INCLUDE_DIRS include
    REQUIRES ${requires}
)
    
add_prebuilt_library(lib${COMPONENT_NAME} lib${COMPONENT_NAME}.a)
target_link_libraries(${COMPONENT_LIB} INTERFACE lib${COMPONENT_NAME})
