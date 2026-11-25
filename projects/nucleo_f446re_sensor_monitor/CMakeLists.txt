cmake_minimum_required(VERSION 3.22)

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/arm-none-eabi.cmake)

project(nucleo_f446re_sensor_monitor C ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_EXTENSIONS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_C_FLAGS_DEBUG     "-O0 -g3")
set(CMAKE_ASM_FLAGS_DEBUG   "-g3")
set(CMAKE_C_FLAGS_RELEASE   "-Os")
set(CMAKE_ASM_FLAGS_RELEASE "")

set(MCU_FLAGS -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard)

set(FREERTOS_DIR Middlewares/Third_Party/FreeRTOS/Source)

file(GLOB APP_SRCS      CONFIGURE_DEPENDS Core/Src/*.c)
file(GLOB HAL_SRCS      CONFIGURE_DEPENDS Drivers/STM32F4xx_HAL_Driver/Src/*.c)
file(GLOB FREERTOS_SRCS CONFIGURE_DEPENDS ${FREERTOS_DIR}/*.c)

list(APPEND FREERTOS_SRCS
    ${FREERTOS_DIR}/CMSIS_RTOS_V2/cmsis_os2.c
    ${FREERTOS_DIR}/portable/GCC/ARM_CM4F/port.c
    ${FREERTOS_DIR}/portable/MemMang/heap_4.c
)

set(STARTUP_SRC Core/Startup/startup_stm32f446retx.s)

add_executable(${PROJECT_NAME}.elf
    ${APP_SRCS}
    ${HAL_SRCS}
    ${FREERTOS_SRCS}
    ${STARTUP_SRC}
)

set_source_files_properties(${STARTUP_SRC} PROPERTIES
    COMPILE_OPTIONS "-x;assembler-with-cpp"
)

target_include_directories(${PROJECT_NAME}.elf PRIVATE
    Core/Inc
    Drivers/STM32F4xx_HAL_Driver/Inc
    Drivers/STM32F4xx_HAL_Driver/Inc/Legacy
    Middlewares/Third_Party/FreeRTOS/Source/include
    Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2
    Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F
    Drivers/CMSIS/Device/ST/STM32F4xx/Include
    Drivers/CMSIS/Include
)

target_compile_definitions(${PROJECT_NAME}.elf PRIVATE
    STM32F446xx
    USE_HAL_DRIVER
    $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:Debug>:APP_DEBUG_COMMANDS>
)

target_compile_options(${PROJECT_NAME}.elf PRIVATE
    ${MCU_FLAGS}
    --specs=nano.specs
    $<$<COMPILE_LANGUAGE:C>:-ffunction-sections>
    $<$<COMPILE_LANGUAGE:C>:-fdata-sections>
    $<$<COMPILE_LANGUAGE:C>:-Wall>
    $<$<COMPILE_LANGUAGE:C>:-fstack-usage>
)

target_link_options(${PROJECT_NAME}.elf PRIVATE
    ${MCU_FLAGS}
    -T${CMAKE_CURRENT_SOURCE_DIR}/STM32F446RETX_FLASH.ld
    --specs=nano.specs
    --specs=nosys.specs
    -Wl,--gc-sections
    -Wl,-Map=${PROJECT_NAME}.map
    -static
)

target_link_libraries(${PROJECT_NAME}.elf PRIVATE
    -Wl,--start-group
    c
    m
    -Wl,--end-group
)

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}.elf>
)