set(requires driver qmsd_utils)

idf_component_register(
	INCLUDE_DIRS .
)

add_prebuilt_library(lib${COMPONENT_NAME} lib${COMPONENT_NAME}.a REQUIRES ${requires})
target_link_libraries(${COMPONENT_LIB} INTERFACE lib${COMPONENT_NAME})
