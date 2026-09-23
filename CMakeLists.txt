cmake_minimum_required(VERSION 3.13)
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
project(famicom_hdmi C CXX ASM)
set(PICO_BOARD waveshare_rp2350_pizero)
set(PICO_PLATFORM rp2350)
pico_sdk_init()

if(DEFINED ENV{PICODVI_PATH})
  set(PICODVI_PATH $ENV{PICODVI_PATH})
else()
  set(PICODVI_PATH ${CMAKE_CURRENT_LIST_DIR}/PicoDVI)
endif()

add_subdirectory(${PICODVI_PATH}/software/libdvi libdvi)

add_executable(famicom_hdmi main.c)
pico_generate_pio_header(famicom_hdmi ${CMAKE_CURRENT_LIST_DIR}/ppu.pio)

target_include_directories(famicom_hdmi PRIVATE
  ${PICODVI_PATH}/software/include
  ${PICODVI_PATH}/software/libdvi
  ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(famicom_hdmi 
  pico_stdlib hardware_pio hardware_clocks hardware_dma pico_multicore libdvi
)

pico_add_extra_outputs(famicom_hdmi)