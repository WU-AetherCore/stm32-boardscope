# 桌面回归测试：编译真实串口管理器，以模拟 HAL 验证状态机。
cmake_minimum_required(VERSION 3.21)
project(BoardAppHostTests LANGUAGES C)
set(CMAKE_C_STANDARD 11)
enable_testing()
add_executable(uart_test uart_test.c ../Drivers/WU/BoardApp/src/board_uart.c)
target_include_directories(uart_test PRIVATE stubs ../Drivers/WU/BoardApp/include)
target_compile_options(uart_test PRIVATE -Wall -Wextra -Werror)
add_test(NAME uart_manager COMMAND uart_test)

find_package(Python3 COMPONENTS Interpreter)
if(Python3_FOUND)
    add_test(NAME telemetry_protocol COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/protocol_test.py)
endif()
