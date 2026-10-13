cmake_minimum_required(VERSION 3.22)

include(${CMAKE_CURRENT_LIST_DIR}/qpm_defines.cmake)

project(${COMPILE_ID})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED 20)

add_compile_definitions(
    MOD_ID="${MOD_ID}"
    VERSION="${VERSION}"
    UNITY_2021
    HAS_CODEGEN
)

add_library(
    ${COMPILE_ID}
    SHARED
    src/main.cpp
)

target_include_directories(${COMPILE_ID} PRIVATE .)
target_include_directories(${COMPILE_ID} PRIVATE include)
target_include_directories(${COMPILE_ID} PRIVATE shared)

include(${CMAKE_CURRENT_LIST_DIR}/extern.cmake)
include(${EXTERN_DIR}/includes/beatsaber-hook/shared/inline-hook/cmake/inline-hook.cmake)

target_link_libraries(${COMPILE_ID} PRIVATE -llog)
