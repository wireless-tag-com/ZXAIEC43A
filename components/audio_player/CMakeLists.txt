set(requires esp_audio_codec audio_hal audio_stream qmsd_audio qmsd_utils gx8006_protocol)

idf_component_register(
	INCLUDE_DIRS include
)

add_prebuilt_library(lib${COMPONENT_NAME} lib${COMPONENT_NAME}.a REQUIRES ${requires})
target_link_libraries(${COMPONENT_LIB} INTERFACE lib${COMPONENT_NAME})
