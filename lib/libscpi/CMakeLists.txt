add_library(scpi STATIC)

target_sources(scpi PRIVATE
    src/error.c
    src/expression.c
    src/ieee488.c
    src/lexer.c
    src/minimal.c
    src/parser.c
    src/units.c
    src/utils.c
    src/fifo.c
)

# Ensure the header is accessible to the main project
target_include_directories(scpi PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_compile_options(scpi PRIVATE -fstack-usage)
target_compile_definitions(scpi PUBLIC USE_FULL_ERROR_LIST)
