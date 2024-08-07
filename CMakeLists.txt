cmake_minimum_required(VERSION 3.13)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(PicoWS2812B C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(PICO_BOARD pico_w)
set(CMAKE_BUILD_TYPE Release)

pico_sdk_init()

add_executable(${PROJECT_NAME} src/main.cpp src/tcp_server.cpp src/ws2812b.cpp)

target_link_libraries(${PROJECT_NAME} 
    pico_stdlib
    pico_time
    hardware_pio
    hardware_clocks
    hardware_timer
    hardware_sync
    hardware_flash
    pico_cyw43_arch_lwip_poll
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src
)

pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_SOURCE_DIR}/src/ws2812b.pio)

pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

pico_add_extra_outputs(${PROJECT_NAME})