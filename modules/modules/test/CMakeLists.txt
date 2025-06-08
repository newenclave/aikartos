set(MODULE_NAME test)

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
    -nostdlib 
    -nodefaultlibs 
    -nostartfiles
    -mcpu=${MCPU} 
    -mthumb 
    -fno-exceptions 
    -fno-rtti 
    -ffreestanding
    -Os
    -fno-threadsafe-statics
    -std=gnu++20
    -mfpu=fpv4-sp-d16 -mfloat-abi=hard
    -ffunction-sections 
    -fno-use-cxa-atexit 
    -fstack-usage 
    -fcyclomatic-complexity
    -U_GLIBCXX_HAS_GTHREADS
    ${PLATFORM_DEFINES}
)

target_link_options(${MODULE_NAME}_elf PRIVATE -T ${LD_SCRIPT} "-Wl,--emit-relocs" -Os)

set_target_properties(${MODULE_NAME}_elf PROPERTIES
    OUTPUT_NAME ${MODULE_NAME}.elf
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build
)

set(ELF_OUTPUT ${CMAKE_BINARY_DIR}/build/${MODULE_NAME}.elf)
set(BIN_RAW ${CMAKE_BINARY_DIR}/build/${MODULE_NAME}.bin)
set(RESULT_OUTPUT ${CMAKE_BINARY_DIR}/build/${MODULE_NAME}.aika)

add_custom_command(
    OUTPUT ${BIN_RAW}
    COMMAND ${CMAKE_OBJCOPY}
        -O binary
        -j .text -j .rodata -j .data -j .bss
        ${ELF_OUTPUT} ${BIN_RAW}
    DEPENDS ${MODULE_NAME}_elf
    COMMENT "[+] Generating raw binary for ${MODULE_NAME}"
)

set(PYTHON_SCRIPT ${CMAKE_SOURCE_DIR}/build/scripts/create_bin.py)

if(MODULE_DESCRIPTION)
    set(MODULE_DESCRIPTION_ARG "-d ${MODULE_DESCRIPTION}")
else()
    set(MODULE_DESCRIPTION_ARG "")
endif()

add_custom_command(
    OUTPUT ${RESULT_OUTPUT}
    COMMAND python ${PYTHON_SCRIPT}
        -e ${ELF_OUTPUT}
        -o ${RESULT_OUTPUT}
        -i ${BIN_RAW}
        ${MODULE_DESCRIPTION_ARG}
        --verbose
    DEPENDS ${BIN_RAW}
    COMMENT "[+] Creating AIKA binary for ${MODULE_NAME}"
)

add_custom_target(${MODULE_NAME}_bin ALL DEPENDS ${RESULT_OUTPUT})

add_custom_command(
    TARGET ${MODULE_NAME}_bin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${MODULES_OUTPUT_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${RESULT_OUTPUT} ${MODULES_OUTPUT_DIR}/${MODULE_NAME}.aika
    COMMENT "[+] Copying ${MODULE_NAME}.bin to ${MODULES_OUTPUT_DIR}"
)
