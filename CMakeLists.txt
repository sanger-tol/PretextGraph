cmake_minimum_required(VERSION 3.20)

project(
    PretextGraph 
    LANGUAGES CXX C
    VERSION 0.1.0
)

add_compile_definitions(PV="${PROJECT_VERSION}")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

include_directories(
    ${CMAKE_SOURCE_DIR}/libdeflate
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/.
)
link_directories(${CMAKE_SOURCE_DIR}/libdeflate)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(DEBUG)
    message(STATUS "Debug version")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -g")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g")
else()
    message(STATUS "Release version")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
endif()


# UCSC bigWigToBedGraph, downloaded for this OS and placed next to PretextGraph.
set(UCSC_BIGWIGTOBEDGRAPH "${CMAKE_BINARY_DIR}/bigWigToBedGraph")
add_custom_command(
    OUTPUT "${UCSC_BIGWIGTOBEDGRAPH}"
    COMMAND "${CMAKE_SOURCE_DIR}/scripts/fetch_ucsc_bigWigToBedGraph.sh" "${UCSC_BIGWIGTOBEDGRAPH}"
    COMMENT "Fetching UCSC bigWigToBedGraph"
    VERBATIM
)
add_custom_target(ucsc_bigWigToBedGraph ALL DEPENDS "${UCSC_BIGWIGTOBEDGRAPH}")

# SIMD extensions. x86 builds one binary per instruction set. arm64 builds one native binary.
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64|ARM64)$" OR CMAKE_OSX_ARCHITECTURES STREQUAL "arm64")
    set(EXTENSIONS ".arm64")
else()
    set(EXTENSIONS
        ".avx2,-mavx2"
        ".avx,-mavx"
        ".sse42,-msse4.2"
        ".sse41,-msse4.1"
    )
endif()
foreach(ext_pair ${EXTENSIONS})
    if(ext_pair MATCHES ",")
        string(REPLACE "," ";" ext_list "${ext_pair}")
        list(GET ext_list 0 EXT_SUFFIX)
        list(GET ext_list 1 EXT_FLAGS)
    else()
        set(EXT_SUFFIX "${ext_pair}")
        set(EXT_FLAGS "")
    endif()
    set(TARGET_NAME "PretextGraph${EXT_SUFFIX}")
    
    add_executable(${TARGET_NAME} ${CMAKE_SOURCE_DIR}/PretextGraph.cpp)
    if(EXT_FLAGS)
        target_compile_options(${TARGET_NAME} PRIVATE ${EXT_FLAGS})
    endif()
    target_link_libraries(
        ${TARGET_NAME}
        PRIVATE
        pthread
        deflate
    )
endforeach()


# ==== SIMD test ====
add_executable(PretextGraph
    SIMDExtensionWrapper.cpp
)
target_compile_definitions(PretextGraph PRIVATE ProgramName="PretextGraph")

include(CTest)
enable_testing()
add_test(NAME TEST COMMAND "PretextGraph")

# ==== Pack ====


set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_IGNORE_FILES
  ".*/build_cmake/;"
  ".*/builddir/;"
  ".*/data_for_test/;"
  ".*/logs/;"
  ".*/test_py/;"
  ".*/pretext_test$;"
  ".*/PretextGraph$;"
  ".*/test$;"
  "/\\\\.git/;"
  "/\\\\.vscode/;"
  "\\\\.swp$;"
  "\\\\.user$;"
  "\\\\.DS_Store$;"
)
message(STATUS "CPack ignored files: ${CPACK_SOURCE_IGNORE_FILES}")
include(CPack)


