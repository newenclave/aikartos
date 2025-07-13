cmake_minimum_required(VERSION 3.13)
set(MODULE_NAME rust_module)

project(${MODULE_NAME} C)

include(${CMAKE_SOURCE_DIR}/cmake/aika_create_module.cmake)

set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)

set(RUST_TARGET thumbv7em-none-eabi)
set(RUSTFLAGS -C opt-level=z -C panic=abort)

set(RUST_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/lib.rs)
set(RUST_OBJ ${CMAKE_BINARY_DIR}/build/${MODULE_NAME}.o)
set(ELF_OUT ${CMAKE_BINARY_DIR}/build/${MODULE_NAME}.elf)

set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/src/module.ld)

add_custom_command(
  OUTPUT ${RUST_OBJ}
  COMMAND rustc
    --crate-type=lib
    --target=${RUST_TARGET}
    ${RUSTFLAGS}
    --emit=obj
    -o ${RUST_OBJ}
    ${RUST_SRC}
  DEPENDS ${RUST_SRC}
  COMMENT "Compiling Rust to .o"
)

add_custom_command(
  OUTPUT ${ELF_OUT}
  COMMAND arm-none-eabi-ld
    -T ${LINKER_SCRIPT}
    --emit-relocs
    -o ${ELF_OUT}
    ${RUST_OBJ}
  DEPENDS ${RUST_OBJ} ${LINKER_SCRIPT}
  COMMENT "Linking module.elf"
)

add_custom_target(build_module ALL
  DEPENDS ${ELF_OUT}
)

set(BIN_RAW ${CMAKE_BINARY_DIR}/build/${MODULE_NAME}.bin)
set(RESULT_OUTPUT ${CMAKE_BINARY_DIR}/build/${MODULE_NAME}.aika)

aika_create_module(
    ${MODULE_NAME}
    ${ELF_OUT}
    ${BIN_RAW}
    ${RESULT_OUTPUT}
    ""
)
