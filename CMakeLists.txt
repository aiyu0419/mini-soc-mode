cmake_minimum_required(VERSION 3.20)

project(
    mini_soc_model
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SystemCLanguage CONFIG REQUIRED)

# ---------------------------------
# SoC model library
# ---------------------------------

add_library(soc_model
    src/memory.cpp
    src/interconnect.cpp
    src/dma.cpp
    src/accelerator.cpp
)

target_include_directories(
    soc_model
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(
    soc_model
    PUBLIC
        SystemC::systemc
)
# ---------------------------------
# Main simulator
# ---------------------------------

add_executable(soc_sim
    src/main.cpp
)

target_link_libraries(soc_sim
    PRIVATE
        soc_model
)

target_compile_options(
    soc_sim
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)
# ---------------------------------
# Tests
# ---------------------------------

enable_testing()

add_subdirectory(tests)
