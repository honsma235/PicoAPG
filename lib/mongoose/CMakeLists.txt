add_library(mongoose STATIC mongoose.c)

# Ensure the header is accessible to the main project
target_include_directories(mongoose PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Link mandatory Pico SDK libraries (like stdlib or cyw43 for Pico W)
target_link_libraries(mongoose
    pico_stdlib
    pico_rand
)

# Prevent extreme optimization from breaking OTA
target_compile_options(mongoose PRIVATE -fno-tree-loop-distribute-patterns -fstack-usage)
