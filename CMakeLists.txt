cmake_minimum_required(VERSION 3.20)

set(CMAKE_C_COMPILER "gcc-14")
set(CMAKE_CXX_COMPILER "g++-14")

project(Gzip)

set(CMAKE_CXX_STANDARD 23)
add_compile_options(-Wall -Wextra)

set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

set(CMAKE_COMPILE_WARNING_AS_ERROR ON)


add_executable(gzip main.cpp)

set_target_properties(gzip PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)

target_include_directories(gzip PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)