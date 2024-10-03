cmake_minimum_required(VERSION 3.0)

project(PretextGraph LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 11)

# set(CMAKE_BUILD_TYPE Debug)

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

