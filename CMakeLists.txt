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

add_executable(
    soc_sim
    src/main.cpp
)

target_link_libraries(
    soc_sim
    PRIVATE
        SystemC::systemc
)

target_compile_options(
    soc_sim
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)
