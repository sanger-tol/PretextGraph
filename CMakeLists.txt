cmake_minimum_required(VERSION 3.0)

project(
    PretextGraph 
    LANGUAGES CXX C
    VERSION 0.0.8
)

set(CMAKE_CXX_STANDARD 11)


link_directories(
    ${CMAKE_SOURCE_DIR}/libdeflate
)
include_directories(
    wrapper
    libdeflate
    src
    .
)
add_executable(PretextGraph PretextGraph.cpp)
target_compile_definitions( PretextGraph PRIVATE PV=${PROJECT_VERSION})


target_link_libraries(
    PretextGraph
    PRIVATE
    pthread
    deflate
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Debug version")
    target_compile_definitions(PretextGraph PRIVATE DEBUG)
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
else()
    message(STATUS "Release version")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
endif()

