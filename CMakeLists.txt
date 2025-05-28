cmake_minimum_required(VERSION 3.20)
project(aikartos C CXX ASM)


if(NOT DEFINED PLATFORM)
  message(FATAL_ERROR "Use -DPLATFORM=f411 or -DPLATFORM=h753")
endif()


# C:\soft\ST\STM32Cube_FW_F4\STM32Cube_FW_F4_V1.28.0\Drivers
if(NOT DEFINED CMSIS_DIRECTORY)
  set(CMSIS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/CMSIS")
  message(STATUS "CMSIS_DIRECTORY not defined, using default: ${CMSIS_DIRECTORY}")
endif()

if(PLATFORM STREQUAL "f411")
  set(MCPU "cortex-m4")
  set(PLATFORM_DEFINES "-DSTM32F411xE -DPLATFORM_f411_CORE")
  set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/aikartos/platform/f411/linker/link_flash.ld")
elseif(PLATFORM STREQUAL "h753")
  set(MCPU "cortex-m7")
  set(PLATFORM_DEFINES "-DSTM32H753xx -DPLATFORM_h753_CORE")
  set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/aikartos/src/aikartos/platform/h753/linker/link_flash.ld")
else()
  message(FATAL_ERROR "Unknown PLATFORM: ${PLATFORM}")
endif()

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
set(TOOLCHAIN_PREFIX arm-none-eabi-)
set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}g++)
set(CMAKE_ASM_COMPILER ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_OBJCOPY ${TOOLCHAIN_PREFIX}objcopy)


add_compile_options(
  -mcpu=${MCPU}
  -mthumb
  -Wall -Wextra -Wno-unused-parameter
  -ffreestanding
  -fno-exceptions
  -fno-rtti
  -fdata-sections -ffunction-sections
  ${PLATFORM_DEFINES}
)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/aikartos/inc
  ${CMAKE_CURRENT_SOURCE_DIR}/platform/${PLATFORM}
  ${CMSIS_DIRECTORY}/Include
  ${CMSIS_DIRECTORY}/Device/ST/STM32${PLATFORM}xx/Include
)

file(GLOB_RECURSE SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/aikartos/src/aikartos/kernel/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/aikartos/src/aikartos/memory/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/aikartos/src/aikartos/platform/${PLATFORM}/device/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/aikartos/src/aikartos/platform/${PLATFORM}/startup/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/aikartos/src/aikartos/platform/${PLATFORM}/startup/*.c
  ${CMAKE_CURRENT_SOURCE_DIR}/aikartos/src/sys/*.c
  ${CMAKE_CURRENT_SOURCE_DIR}/aikartos/src/tests/*.cpp
)

add_executable(aikartos.elf ${SOURCES})

target_link_options(aikartos.elf PRIVATE
  -T${LINKER_SCRIPT}
  -nostdlib -Wl,--gc-sections
)

add_custom_command(TARGET aikartos.elf POST_BUILD
  COMMAND ${CMAKE_OBJCOPY} -O binary aikartos.elf aikartos.bin
)
