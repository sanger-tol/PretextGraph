cmake_minimum_required(VERSION 3.0)

project(PretextGraph LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 11)

set(version "0.0.6")

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
    message(STATUS "No build type selected, default to Debug")
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

link_directories(
    ${CMAKE_SOURCE_DIR}/libdeflate
)
add_executable(PretextGraph PretextGraph.cpp)

target_include_directories(
    PretextGraph
    PRIVATE
    wrapper
    libdeflate
)

target_link_libraries(
    PretextGraph
    PRIVATE
    pthread
    deflate
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(PretextGraph PRIVATE DEBUG)
endif()

target_compile_definitions(PretextGraph PRIVATE PV="${version}")
