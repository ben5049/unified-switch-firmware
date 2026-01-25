set(NANOPB_DIR "${CMAKE_SOURCE_DIR}/libraries/nanopb")

add_library(nanopb STATIC
    ${NANOPB_DIR}/pb_common.c
    ${NANOPB_DIR}/pb_encode.c
    ${NANOPB_DIR}/pb_decode.c
)

target_include_directories(nanopb PUBLIC
    ${NANOPB_DIR}
)

target_compile_options(nanopb PRIVATE
    -ffunction-sections
    -fdata-sections
)
