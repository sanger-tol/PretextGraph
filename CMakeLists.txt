cmake_minimum_required(VERSION 3.20)

project(
    PretextGraph 
    LANGUAGES CXX C
    VERSION 0.0.8
)

add_compile_definitions(PV="${PROJECT_VERSION}")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

include_directories(
    ${CMAKE_SOURCE_DIR}/wrapper
    ${CMAKE_SOURCE_DIR}/libdeflate
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/.
)
link_directories(${CMAKE_SOURCE_DIR}/libdeflate)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(DEBUG)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -g")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g")
endif()


# SIMD exntensions
set(EXTENSIONS
    ".avx2,-mavx2"
    ".avx,-mavx"
    ".sse42,-msse4.2"
    ".sse41,-msse4.1"
)
foreach(ext_pair ${EXTENSIONS})
    string(REPLACE "," ";" ext_list "${ext_pair}")
    list(GET ext_list 0 EXT_SUFFIX)
    list(GET ext_list 1 EXT_FLAGS)
    set(TARGET_NAME "PretextGraph${EXT_SUFFIX}")
    
    add_executable(${TARGET_NAME} ${CMAKE_SOURCE_DIR}/PretextGraph.cpp)
    target_compile_options(${TARGET_NAME} PRIVATE ${EXT_FLAGS})
    target_link_libraries(
        ${TARGET_NAME}
        PRIVATE
        pthread
        deflate
    )
endforeach()


# ==== SIMD test ====
add_executable(PretextGraph
    wrapper/SIMDExtensionWrapper.cpp
)
target_compile_definitions(PretextGraph PRIVATE ProgramName="PretextGraph")

include(CTest)
enable_testing()
add_test(NAME TEST COMMAND "PretextGraph")
