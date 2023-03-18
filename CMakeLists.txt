cmake_minimum_required(VERSION 3.22)
include(pico_sdk_import.cmake)

project(ZipZap C CXX ASM)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

pico_sdk_init()

add_executable(ZipZap main.cpp)

pico_generate_pio_header(ZipZap ${CMAKE_CURRENT_LIST_DIR}/zap.pio)

add_subdirectory(pico-ssd1306)

target_link_libraries(ZipZap pico_ssd1306 hardware_i2c hardware_pio hardware_irq pico_stdlib)

pico_add_extra_outputs(ZipZap)
