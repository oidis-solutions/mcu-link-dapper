# * ********************************************************************************************************* *
# *
# * Copyright 2024 NXP
# *
# * SPDX-License-Identifier: BSD-3-Clause
# * The BSD-3-Clause license for this file can be found in the LICENSE.txt file included with this distribution
# * or at https://spdx.org/licenses/BSD-3-Clause.html#licenseText
# *
# * ********************************************************************************************************* *

cmake_minimum_required(VERSION 3.16)
project(webix-dapper)

set(CMAKE_CXX_STANDARD 17)

# add compile definitions for new version, this could be resolved from perl script or from LVM team
if (WIN32)
    message(FATAL_ERROR "Detected platform: WIN. This is not supported yet.\n")
elseif (UNIX)
    message(STATUS "Detected platform: POSIX\n")

    if (APPLE AND NOT (EMSCRIPTEN))
        set(CMAKE_OSX_DEPLOYMENT_TARGET "14.0")
        # position independent code may be enabled also on linux - to be validated
        set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    endif ()

    if (NOT (EMSCRIPTEN))
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/webix-dapper)
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/webix-dapper)
    else ()
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/build_wasm)
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/build_wasm)
    endif ()
endif ()

# unused cli arg flag is for emscripten build on local machine, it is rising error and not sure why
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -Wpedantic -Wextra -Wno-sign-compare -Wno-unused-command-line-argument")

set(WASM_TARGETS "" CACHE INTERNAL "List of global targets")

add_subdirectory(src/wasm)
add_subdirectory(test/wasm)

message(STATUS "Global targets: ${WASM_TARGETS}")
add_custom_target(ALL_TARGETS
    DEPENDS ${WASM_TARGETS}
)
