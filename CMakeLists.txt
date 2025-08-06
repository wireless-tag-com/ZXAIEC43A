# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

set(SOFT_VERSION "v1.0.0")
add_definitions(-DSOFT_VERSION="${SOFT_VERSION}")

set(ENV{QMSD_8MS_PATH} ${CMAKE_SOURCE_DIR}/qmsd_esp32_sdk)

if(NOT DEFINED ENV{ADF_PATH})
    set(ENV{ADF_PATH} "/home/sorz/environment/esp-adf")
endif()

include($ENV{QMSD_8MS_PATH}/component.cmake)
include($ENV{ADF_PATH}/CMakeLists.txt)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(c38006)
