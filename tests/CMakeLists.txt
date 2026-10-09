# 桌面回归测试：编译真实串口管理器，以模拟 HAL 验证状态机。
cmake_minimum_required(VERSION 3.21)
project(BoardAppHostTests LANGUAGES C)
set(CMAKE_C_STANDARD 11)
enable_testing()
set(GRAPH_DIR ../Middlewares/Third_Party/Astra/hal/hal_dreamCore/components/oled/graph_lib/u8g2)
file(GLOB GRAPH_SOURCES ${GRAPH_DIR}/*.c)
list(FILTER GRAPH_SOURCES EXCLUDE REGEX "(u8g2_font_myfont|u8x8_fonts)\\.c$")
add_executable(ui_text_test ui_text_test.c ${GRAPH_SOURCES}
    ../Drivers/WU/BoardApp/src/astra_text.c
    ../Drivers/WU/BoardApp/src/board_font.c
    ../Drivers/WU/BoardApp/src/board_small_font.c)
target_include_directories(ui_text_test PRIVATE ${GRAPH_DIR} ../Drivers/WU/BoardApp/include)
target_compile_options(ui_text_test PRIVATE -ffunction-sections -fdata-sections)
target_link_options(ui_text_test PRIVATE -Wl,--gc-sections)
add_test(NAME ui_text_bounds COMMAND ui_text_test)
add_executable(uart_test uart_test.c ../Drivers/WU/BoardApp/src/board_uart.c)
target_include_directories(uart_test PRIVATE stubs ../Drivers/WU/BoardApp/include)
target_compile_options(uart_test PRIVATE -Wall -Wextra -Werror)
add_test(NAME uart_manager COMMAND uart_test)

find_package(Python3 COMPONENTS Interpreter)
if(Python3_FOUND)
    add_test(NAME telemetry_protocol COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/protocol_test.py)
endif()
