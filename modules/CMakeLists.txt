cmake_minimum_required(VERSION 3.15)
project(AikaRTOS_MODULES C CXX ASM)

# set(CMAKE_C_COMPILER arm-none-eabi-gcc)
# set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
# set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
# set(CMAKE_LINKER arm-none-eabi-ld)

if(PLATFORM STREQUAL "f411")
  set(MCPU "cortex-m4")
  set(CMSIS_DEVICE_NAME "F4")
  set(PLATFORM_DEFINES -DSTM32F411xE -DPLATFORM_f411_CORE)
elseif(PLATFORM STREQUAL "h753")
  set(CMSIS_DEVICE_NAME "H7")
  set(MCPU "cortex-m7")
  set(PLATFORM_DEFINES -DSTM32H753xx -DPLATFORM_h753_CORE)
else()
  message(FATAL_ERROR "Unknown PLATFORM: ${PLATFORM}")
endif()

# ST\STM32Cube_FW_F4\STM32Cube_FW_F4_V1.28.0\Drivers/CMSIS
if(NOT DEFINED CMSIS_DIRECTORY)
  set(CMSIS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/CMSIS")
  message(STATUS "CMSIS_DIRECTORY not defined, using default: ${CMSIS_DIRECTORY}")
endif()

set(SDK_DIR ${PROJECT_SOURCE_DIR}/sdk)
set(INC_DIR ${PROJECT_SOURCE_DIR}/../aikartos/inc)
set(CMSI_DIRS 
  ${CMSIS_DIRECTORY}/Include
  ${CMSIS_DIRECTORY}/Device/ST/STM32${CMSIS_DEVICE_NAME}xx/Include
)

set(MODULES_OUTPUT_DIR ${CMAKE_BINARY_DIR}/modules/bin)
file(MAKE_DIRECTORY ${MODULES_OUTPUT_DIR})

add_subdirectory(modules)
