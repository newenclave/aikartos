set(MODULE_NAME test)

include(${CMAKE_SOURCE_DIR}/cmake/aika_create_module.cmake)

set(SOURCES
    src/module.cpp
)

set(LD_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/src/module.ld)
set(DESCRIPTION_FILE ${CMAKE_CURRENT_SOURCE_DIR}/description.txt)

if(EXISTS ${DESCRIPTION_FILE})
    file(READ ${DESCRIPTION_FILE} MODULE_DESCRIPTION)
    string(STRIP "${MODULE_DESCRIPTION}" MODULE_DESCRIPTION)
endif()

include_directories(
    ${SDK_DIR}
    ${INC_DIR}
    ${CMSI_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(${MODULE_NAME}_elf ${SOURCES})

target_compile_options(${MODULE_NAME}_elf PRIVATE
    #-nostdlib 
    -nodefaultlibs 
    -nostartfiles
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

target_link_options(${MODULE_NAME}_elf PRIVATE
    -mcpu=${MCPU}
    -Wl,--gc-sections 
        -nodefaultlibs 
        -static 
        --specs=nano.specs 
        -mfpu=fpv4-sp-d16 
        -mfloat-abi=hard 
        -mthumb 
    -Wl,--start-group 
        -lc -lm -lgcc
    -Wl,--end-group
)

target_link_options(${MODULE_NAME}_elf PRIVATE -T ${LD_SCRIPT} "-Wl,--emit-relocs" -Os)

set_target_properties(${MODULE_NAME}_elf PROPERTIES
    OUTPUT_NAME ${MODULE_NAME}.elf
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build
)

set(ELF_OUTPUT ${CMAKE_BINARY_DIR}/build/${MODULE_NAME}.elf)
set(BIN_RAW ${CMAKE_BINARY_DIR}/build/${MODULE_NAME}.bin)
set(RESULT_OUTPUT ${CMAKE_BINARY_DIR}/build/${MODULE_NAME}.aika)

aika_create_module(
    ${MODULE_NAME}
    ${ELF_OUTPUT}
    ${BIN_RAW}
    ${RESULT_OUTPUT}
    ${MODULE_DESCRIPTION}
)

