# CMake include file

# Add more sources
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/lwmem_test_calloc.c
)

# Options file
set(LWMEM_OPTS_FILE ${CMAKE_CURRENT_LIST_DIR}/lwmem_opts.h)

# This test hardcodes 32-bit size_t assumptions
set(LWMEM_TEST_ARCH 32)
