cmake_minimum_required(VERSION 3.11)

project(
    Minicord
    VERSION 0.1.0
    DESCRIPTION "Simple Discord user client on C++"
    HOMEPAGE_URL "https://github.com/thehatkid/Minicord"
    LANGUAGES CXX C
)

# Project options
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_COLOR_DIAGNOSTICS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(MINICORD_TARGET "minicord" CACHE STRING "Set target and executable name")

option(MINICORD_USE_SSE2 "Enable SSE2 optimization for RapidJSON (default: ON)" ON)
option(MINICORD_USE_NEON "Enable NEON optimization for RapidJSON (default: OFF)" OFF)

set(CMAKE_CXX_FLAGS "-Wall -Wextra -pedantic")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)

# Platform build detection
if(WIN32)
    if(MINGW)
        # Get current MSYSTEM environment
        string(TOLOWER "$ENV{MSYSTEM}" MINICORD_MSYSTEM)

        message(STATUS "Building for Windows (${MINICORD_MSYSTEM})")

        # Set dependencies prefix path
        set(MINICORD_PREFIX "${CMAKE_SOURCE_DIR}/windows/build-${MINICORD_MSYSTEM}")
        set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "${MINICORD_PREFIX}")
        set(ENV{PKG_CONFIG_PATH} "${MINICORD_PREFIX}/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")

        # Static libgcc, libstdc and libwinpthread on Windows
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++ -static")
    elseif(MSVC)
        message(STATUS "Building for Windows (MSVC)")
        message(STATUS "Building not in MinGW environment (WIP)")
    else()
        message(STATUS "Building for Windows (Other)")
        message(STATUS "Building not in MinGW environment (WIP)")
    endif()

    # Add project definitions
    add_compile_definitions(BUILD_WINDOWS)
    add_compile_definitions(WIN32_LEAN_AND_MEAN)
elseif(LINUX)
    set(MINICORD_ARCH "${CMAKE_HOST_SYSTEM_PROCESSOR}")

    message(STATUS "Building for Linux (${MINICORD_ARCH})")

    # Set dependencies prefix path
    set(MINICORD_PREFIX "${CMAKE_SOURCE_DIR}/linux/build-${MINICORD_ARCH}")
    set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "${MINICORD_PREFIX}")
    set(ENV{PKG_CONFIG_PATH} "${MINICORD_PREFIX}/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")

    # Add project definitions
    add_compile_definitions(BUILD_LINUX)
else()
    message(WARNING "Cannot detect this platform (WIP)")
endif()

# Minicord source files
set(MINICORD_SOURCES
    src/main.cpp
    src/etc/utils.cpp
    src/gateway/gateway.cpp
)

# Minicord include directories
set(MINICORD_INCLUDES
    src
)

# Minicord executable target
add_executable(${MINICORD_TARGET} ${MINICORD_SOURCES})

target_include_directories(${MINICORD_TARGET} PUBLIC ${MINICORD_INCLUDES})
target_include_directories(${MINICORD_TARGET} PRIVATE "${MINICORD_PREFIX}/include")

find_package(Threads)
if(Threads_FOUND)
    target_link_libraries(${MINICORD_TARGET} Threads::Threads)
endif()

find_package(RapidJSON REQUIRED)
if(RapidJSON_FOUND)
    target_compile_definitions(${MINICORD_TARGET} PUBLIC RAPIDJSON_HAS_STDSTRING=1)

    if(MINICORD_USE_SSE2)
        target_compile_definitions(${MINICORD_TARGET} PUBLIC RAPIDJSON_SSE2)
    endif()
    if(MINICORD_USE_NEON)
        target_compile_definitions(${MINICORD_TARGET} PUBLIC RAPIDJSON_NEON)
    endif()
endif()

find_package(ZLIB REQUIRED)
find_package(MbedTLS REQUIRED)
find_package(ixwebsocket REQUIRED)

target_link_libraries(${MINICORD_TARGET} ${ZLIB_LIBRARY})
target_link_libraries(${MINICORD_TARGET} RapidJSON)
target_link_libraries(${MINICORD_TARGET} MbedTLS::mbedcrypto)
target_link_libraries(${MINICORD_TARGET} MbedTLS::mbedx509)
target_link_libraries(${MINICORD_TARGET} MbedTLS::mbedtls)
target_link_libraries(${MINICORD_TARGET} ixwebsocket::ixwebsocket)

message(STATUS "${PROJECT_NAME} version ${PROJECT_VERSION} (${CMAKE_BUILD_TYPE})")
