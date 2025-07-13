function(aika_create_module MODULE_NAME ELF_PATH BIN_PATH OUTPUT_PATH DESCRIPTION)
    set(PYTHON_SCRIPT ${CMAKE_SOURCE_DIR}/build/scripts/create_bin.py)

    if(DESCRIPTION)
        set(DESC_ARG -d "${DESCRIPTION}")
    else()
        set(DESC_ARG "")
    endif()

    set(TARGET_NAME ${MODULE_NAME}_bin)

    add_custom_command(
        OUTPUT ${BIN_PATH}
        COMMAND ${CMAKE_OBJCOPY}
            -O binary
            -j .text -j .rodata -j .data -j .bss
            ${ELF_PATH} ${BIN_PATH}
        DEPENDS ${ELF_PATH}
        COMMENT "[+] Generating raw binary for ${MODULE_NAME}"
    )

    add_custom_command(
        OUTPUT ${OUTPUT_PATH}
        COMMAND python ${PYTHON_SCRIPT}
            -e ${ELF_PATH}
            -o ${OUTPUT_PATH}
            -i ${BIN_PATH}
            ${DESC_ARG}
            --verbose
        DEPENDS ${BIN_PATH}
        COMMENT "[+] Creating AIKA binary for ${MODULE_NAME}"
    )

    add_custom_target(${TARGET_NAME} ALL DEPENDS ${OUTPUT_PATH})

    add_custom_command(
        TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${MODULES_OUTPUT_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${OUTPUT_PATH} ${MODULES_OUTPUT_DIR}/${MODULE_NAME}.aika
        COMMENT "[+] Copying ${MODULE_NAME}.aika to ${MODULES_OUTPUT_DIR}"
    )
endfunction()