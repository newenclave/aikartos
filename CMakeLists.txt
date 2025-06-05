cmake_minimum_required(VERSION 3.20)
project(aikartos C CXX ASM)


if(NOT DEFINED PLATFORM)
  message(FATAL_ERROR "Use -DPLATFORM=f411 or -DPLATFORM=h753")
endif()

# ST\STM32Cube_FW_F4\STM32Cube_FW_F4_V1.28.0\Drivers/CMSIS
if(NOT DEFINED CMSIS_DIRECTORY)
  set(CMSIS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/CMSIS")
  message(STATUS "CMSIS_DIRECTORY not defined, using default: ${CMSIS_DIRECTORY}")
endif()

if(PLATFORM STREQUAL "f411")
  set(MCPU "cortex-m4")
  set(CMSIS_DEVICE_NAME "F4")
  set(PLATFORM_DEFINES -DSTM32F411xE -DPLATFORM_f411_CORE)
  set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/aikartos/src/aikartos/platform/f411/linker/link_flash.ld")
elseif(PLATFORM STREQUAL "h753")
  set(CMSIS_DEVICE_NAME "H7")
  set(MCPU "cortex-m7")
  set(PLATFORM_DEFINES -DSTM32H753xx -DPLATFORM_h753_CORE)
  set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/aikartos/src/aikartos/platform/h753/linker/link_flash.ld")
else()
  message(FATAL_ERROR "Unknown PLATFORM: ${PLATFORM}")
endif()

message("CMSIS_DEVICE_NAME == ${CMSIS_DEVICE_NAME}")

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
set(TOOLCHAIN_PREFIX arm-none-eabi-)
set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}g++)
set(CMAKE_ASM_COMPILER ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_LINKER ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_OBJCOPY ${TOOLCHAIN_PREFIX}objcopy)

add_compile_options(
  -mcpu=${MCPU}
  -mthumb
  -Wall -Wextra -Wno-unused-parameter -Wdouble-promotion
  -fno-exceptions
  -fno-rtti
  -fdata-sections
  -fno-threadsafe-statics
  -std=gnu++20
  -mfpu=fpv4-sp-d16 -mfloat-abi=hard
  -ffunction-sections 
  -fno-use-cxa-atexit 
  -fstack-usage 
  -fcyclomatic-complexity
  -U_GLIBCXX_HAS_GTHREADS
  --specs=nano.specs 
  ${PLATFORM_DEFINES}
)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/aikartos/inc
  ${CMAKE_CURRENT_SOURCE_DIR}/platform/${PLATFORM}
  ${CMSIS_DIRECTORY}/Include
  ${CMSIS_DIRECTORY}/Device/ST/STM32${CMSIS_DEVICE_NAME}xx/Include
)

file(GLOB_RECURSE SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/aikartos/src/aikartos/kernel/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/aikartos/src/aikartos/memory/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/aikartos/src/aikartos/platform/${PLATFORM}/device/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/aikartos/src/aikartos/platform/${PLATFORM}/startup/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/aikartos/src/sys/*.c
  ${CMAKE_CURRENT_SOURCE_DIR}/aikartos/src/tests/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/aikartos/src/*.cpp
)

file(GLOB STARTUP_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/aikartos/src/aikartos/platform/${PLATFORM}/startup/*.s
)

set_source_files_properties(${STARTUP_SOURCES}
  PROPERTIES LANGUAGE ASM
)

set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -x assembler-with-cpp -mcpu=${MCPU} -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard"
)

add_executable(aikartos.elf
  ${SOURCES}
  ${STARTUP_SOURCES}
)

target_sources(aikartos.elf PRIVATE ${STARTUP_SOURCES})

# target_link_libraries(aikartos.elf
#   -lc
#   -lm
#   -lstdc++
#   -lsupc++
#   -lgcc
# )

target_link_options(aikartos.elf PRIVATE
    -mcpu=${MCPU}
    -T${LINKER_SCRIPT}
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/aikartos.map
    -Wl,--gc-sections 
        -nodefaultlibs 
        -static 
        --specs=nano.specs 
        -mfpu=fpv4-sp-d16 
        -mfloat-abi=hard 
        -mthumb 
    -Wl,--start-group 
        -lc -lm -lstdc++ -lsupc++ 
    -Wl,--end-group
)

add_custom_command(TARGET aikartos.elf POST_BUILD
  COMMAND ${CMAKE_OBJCOPY} -O binary aikartos.elf aikartos.bin
)
