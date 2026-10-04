cmake_minimum_required(VERSION 3.21)

# CLion 直接打开工程时也自动选用 ARM 工具链，不依赖桌面编译器。
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/cmake/arm-none-eabi.cmake"
        CACHE FILEPATH "ARM GCC toolchain")
endif()
project(STM32-BoardScope VERSION 1.1.1 LANGUAGES C ASM)
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
option(WU_USE_KK_UI "启用 V1.1 板级 UI" ON)
if(NOT WU_USE_KK_UI)
    message(FATAL_ERROR "V1.1 的串口和板级诊断依赖 BoardApp；旧 OLED 请使用保留的 V1.0 工程。")
endif()
if(WU_USE_KK_UI)
    # 保留旧 OLED 源码供参考，不参与 V1.1 UI 编译，避免两个显示任务冲突。
    list(FILTER FIRMWARE_SOURCES EXCLUDE REGEX "/OLED_U8g2/")
    file(GLOB_RECURSE KK_SOURCES CONFIGURE_DEPENDS
        "Middlewares/Third_Party/KK_UI/src/*.c"
        "Middlewares/Third_Party/KK_OLED/graphics/*.c"
        "Middlewares/Third_Party/KK_OLED/driver/*.c")
    list(APPEND FIRMWARE_SOURCES ${KK_SOURCES})
else()
    list(FILTER FIRMWARE_SOURCES EXCLUDE REGEX "/BoardApp/")
endif()
set(FREERTOS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/Third_Party/FreeRTOS/Source")
file(GLOB FREERTOS_SOURCES CONFIGURE_DEPENDS "${FREERTOS_DIR}/*.c")
add_executable(${PROJECT_NAME}
    ${FIRMWARE_SOURCES} ${FREERTOS_SOURCES}
    ${FREERTOS_DIR}/CMSIS_RTOS/cmsis_os.c
    ${FREERTOS_DIR}/portable/GCC/ARM_CM4F/port.c
    ${FREERTOS_DIR}/portable/MemMang/heap_4.c
    Core/Startup/startup_stm32f407vetx.s)
set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".elf")
if(WU_USE_KK_UI)
    target_compile_definitions(${PROJECT_NAME} PRIVATE WU_USE_KK_UI=1
        KK_UI_REFRESH_MODE=0 INCLUDE_uxTaskGetStackHighWaterMark=1)
    target_include_directories(${PROJECT_NAME} PRIVATE
        Middlewares/Third_Party/KK_UI/include
        Middlewares/Third_Party/KK_OLED/include
        Middlewares/Third_Party/KK_OLED/graphics
        Middlewares/Third_Party/KK_OLED/driver
        Drivers/WU/BoardApp/include)
endif()
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
