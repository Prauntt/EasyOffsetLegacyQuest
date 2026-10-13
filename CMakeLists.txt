cmake_minimum_required(VERSION 3.22)

# Generated automatically by qpm restore
include(${CMAKE_CURRENT_LIST_DIR}/qpm_defines.cmake)

if(DEFINED ENV{ANDROID_NDK_HOME} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{ANDROID_NDK_HOME}/build/cmake/android.toolchain.cmake")
endif()

set(ANDROID_ABI "arm64-v8a")
set(ANDROID_PLATFORM "android-29")
set(ANDROID_STL "c++_static")

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
target_include_directories(${COMPILE_ID} SYSTEM PRIVATE ${EXTERN_DIR}/includes)
target_include_directories(${COMPILE_ID} SYSTEM PRIVATE ${EXTERN_DIR}/includes/bs-cordl/include)

include(${EXTERN_DIR}/includes/beatsaber-hook/shared/inline-hook/cmake/inline-hook.cmake)

target_link_libraries(${COMPILE_ID} PRIVATE -llog)
include(${CMAKE_CURRENT_LIST_DIR}/qpm_shared.cmake)
