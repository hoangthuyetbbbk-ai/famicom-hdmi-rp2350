cmake_minimum_required(VERSION 3.13)
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
project(famicom_hdmi C CXX ASM)

pico_sdk_init()

add_executable(famicom_hdmi main.c)
pico_generate_pio_header(famicom_hdmi ${CMAKE_CURRENT_LIST_DIR}/ppu.pio)

target_link_libraries(famicom_hdmi
    pico_stdlib
    hardware_pio
    hardware_clocks
    pico_multicore
    libdvi
)

pico_add_extra_outputs(famicom_hdmi)
