# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(PicoAPG C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

add_subdirectory(lib/mongoose)
add_subdirectory(lib/libscpi)

# Add executable. Default name is the project name, version 0.1
add_executable(${PROJECT_NAME}
    main.c
    usb_network/usb_network.c
    usb_network/usb_descriptors.c
    scpi_server/scpi_server.c
    scpi_server/scpi-def.c
    ${PICO_TINYUSB_PATH}/lib/networking/rndis_reports.c
)

pico_set_program_name(${PROJECT_NAME} "PicoAPG")
pico_set_program_version(${PROJECT_NAME} "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(${PROJECT_NAME} 0)
pico_enable_stdio_usb(${PROJECT_NAME} 0)

# Add the standard library to the build
target_link_libraries(${PROJECT_NAME}
        pico_stdlib
        pico_unique_id
        tinyusb_device
        mongoose
        scpi
)

# Add the standard include files to the build
target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/usb_network
        ${CMAKE_CURRENT_LIST_DIR}/scpi_server
        ${PICO_TINYUSB_PATH}/lib/networking
)

pico_add_extra_outputs(${PROJECT_NAME})

# Extra build flags (enable if needed)
add_definitions(-DDUAL_CONFIG=1)

target_link_options(${PROJECT_NAME} PUBLIC "-Wl,--print-memory-usage")
target_compile_options(${PROJECT_NAME} PRIVATE -fstack-usage) # Adds stack usage files in build/CMakeFiles/PicoAPG.dir/… as .su files.