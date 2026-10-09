cmake_minimum_required(VERSION 3.21)

# CLion 直接打开工程时也自动选用 ARM 工具链，不依赖桌面编译器。
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/cmake/arm-none-eabi.cmake"
        CACHE FILEPATH "ARM GCC toolchain")
endif()
project(STM32F407VET6-CLion-V1.2 VERSION 1.2.0 LANGUAGES C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()
set(CMAKE_C_FLAGS_DEBUG "-O0 -g3")
set(CMAKE_ASM_FLAGS_DEBUG "-g3")
set(CMAKE_C_FLAGS_RELEASE "-Os -g0")

# HAL 与 FreeRTOS 保留当前 STM32F407 Cortex-M4F 移植配置。
# CONFIGURE_DEPENDS 自动识别新增业务源码；依赖库保持原路径。
file(GLOB_RECURSE FIRMWARE_SOURCES CONFIGURE_DEPENDS
    "Core/Src/*.c" "Drivers/WU/*.c"
    "Drivers/STM32F4xx_HAL_Driver/Src/*.c")
# V1.2 完全停用旧 KK_UI/KK_OLED，旧文件仅保留作迁移参考。
list(FILTER FIRMWARE_SOURCES EXCLUDE REGEX "/(OLED_U8g2|kk_ui_app|kk_ui_port)/")
list(FILTER FIRMWARE_SOURCES EXCLUDE REGEX "kk_ui_(app|port)\\.c$")
set(ASTRA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/Third_Party/Astra")
file(GLOB_RECURSE ASTRA_SOURCES CONFIGURE_DEPENDS
    "${ASTRA_DIR}/astra/ui/*.cpp" "${ASTRA_DIR}/hal/hal.cpp"
    "Drivers/WU/BoardApp/src/*.cpp"
    "${ASTRA_DIR}/hal/hal_dreamCore/components/oled/graph_lib/u8g2/*.c")
list(FILTER ASTRA_SOURCES EXCLUDE REGEX "(u8g2_font_myfont|u8x8_fonts)\\.c$")
list(APPEND FIRMWARE_SOURCES ${ASTRA_SOURCES})
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3")
set(CMAKE_CXX_FLAGS_RELEASE "-Os -g0")
set(FREERTOS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/Third_Party/FreeRTOS/Source")
file(GLOB FREERTOS_SOURCES CONFIGURE_DEPENDS "${FREERTOS_DIR}/*.c")
add_executable(${PROJECT_NAME}
    ${FIRMWARE_SOURCES} ${FREERTOS_SOURCES}
    ${FREERTOS_DIR}/CMSIS_RTOS/cmsis_os.c
    ${FREERTOS_DIR}/portable/GCC/ARM_CM4F/port.c
    ${FREERTOS_DIR}/portable/MemMang/heap_4.c
    Core/Startup/startup_stm32f407vetx.s)
set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".elf")
target_compile_definitions(${PROJECT_NAME} PRIVATE WU_USE_ASTRA_UI=1
    INCLUDE_uxTaskGetStackHighWaterMark=1)
target_include_directories(${PROJECT_NAME} PRIVATE
    Drivers/WU/BoardApp/include "${ASTRA_DIR}"
    "${ASTRA_DIR}/hal/hal_dreamCore/components/oled/graph_lib/u8g2")
target_include_directories(${PROJECT_NAME} PRIVATE
    Core/Inc Drivers/STM32F4xx_HAL_Driver/Inc
    Drivers/STM32F4xx_HAL_Driver/Inc/Legacy
    Drivers/CMSIS/Device/ST/STM32F4xx/Include Drivers/CMSIS/Include
    ${FREERTOS_DIR}/include ${FREERTOS_DIR}/CMSIS_RTOS
    ${FREERTOS_DIR}/portable/GCC/ARM_CM4F)
target_compile_definitions(${PROJECT_NAME} PRIVATE
    USE_HAL_DRIVER STM32F407xx $<$<CONFIG:Debug>:DEBUG>)
set(MCU_FLAGS -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard)
target_compile_options(${PROJECT_NAME} PRIVATE
    ${MCU_FLAGS} --specs=nano.specs
    $<$<COMPILE_LANGUAGE:C>:-ffunction-sections;-fdata-sections;-Wall;-fstack-usage>
    $<$<COMPILE_LANGUAGE:CXX>:-ffunction-sections;-fdata-sections;-Wall;-fstack-usage;-fno-exceptions;-fno-rtti;-fno-threadsafe-statics>
    $<$<COMPILE_LANGUAGE:ASM>:-x;assembler-with-cpp>)
set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/STM32F407VETX_FLASH.ld")
set_property(TARGET ${PROJECT_NAME} APPEND PROPERTY LINK_DEPENDS "${LINKER_SCRIPT}")
target_link_options(${PROJECT_NAME} PRIVATE
    ${MCU_FLAGS} "-T${LINKER_SCRIPT}" --specs=nosys.specs --specs=nano.specs
    -static -Wl,--gc-sections -Wl,--print-memory-usage
    "-Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.map"
    $<$<CONFIG:Debug>:-Wl,-u,_printf_float;-Wl,-u,_scanf_float>)
target_link_libraries(${PROJECT_NAME} PRIVATE -Wl,--start-group c m -Wl,--end-group)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND "${CMAKE_OBJCOPY}" -O ihex $<TARGET_FILE:${PROJECT_NAME}>
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.hex"
    COMMAND "${CMAKE_OBJCOPY}" -O binary $<TARGET_FILE:${PROJECT_NAME}>
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.bin"
    COMMAND "${CMAKE_SIZE}" $<TARGET_FILE:${PROJECT_NAME}>
    BYPRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.hex"
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.bin"
    VERBATIM)

# 独立 flash 目标负责下载和校验；普通编译目标只生成固件。
find_program(STM32_PROGRAMMER_CLI NAMES STM32_Programmer_CLI)
set(STLINK_SERIAL "" CACHE STRING "Optional ST-Link serial for the flash target")
if(STM32_PROGRAMMER_CLI)
    set(STLINK_ARGS port=SWD mode=UR freq=4000)
    if(STLINK_SERIAL)
        list(APPEND STLINK_ARGS "sn=${STLINK_SERIAL}")
    endif()
    add_custom_target(flash
        COMMAND "${STM32_PROGRAMMER_CLI}" -c ${STLINK_ARGS}
            -w $<TARGET_FILE:${PROJECT_NAME}> -v -rst
        DEPENDS ${PROJECT_NAME}
        USES_TERMINAL VERBATIM
        COMMENT "Download firmware through ST-Link, verify and reset")
endif()

# UI 与中断发送走热路径：避免 Debug 的 -O0 导致字模解码和每字节 IRQ 变慢。
set_source_files_properties(${ASTRA_SOURCES}
    Drivers/WU/BoardApp/src/astra_display.c
    Drivers/WU/BoardApp/src/astra_text.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_i2c.c
    PROPERTIES COMPILE_OPTIONS "-O2")
