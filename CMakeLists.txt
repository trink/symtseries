# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project(symtseries)
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Symbolic analysis for time series data")
set(CPACK_PACKAGE_VERSION_MAJOR 0)
set(CPACK_PACKAGE_VERSION_MINOR 1)

if(CMAKE_HOST_UNIX)
    # Compiler options: http://gcc.gnu.org/onlinedocs/gcc/Invoking-GCC.html#Invoking-GCC
    set(CMAKE_C_FLAGS   "-std=c99 -pedantic -Werror -Wall -Wextra -fPIC")
    set(CMAKE_C_FLAGS_DEBUG     "-g")

    set(CMAKE_C_FLAGS_RELEASE   "-O2 -DNDEBUG")

    set(CMAKE_C_FLAGS_PROFILE   "${CMAKE_C_FLAGS_RELEASE} -g -pg")

    set(CPACK_GENERATOR         "TGZ")

    set(CMAKE_SKIP_BUILD_RPATH              FALSE)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH      FALSE)
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH   FALSE)
    set(CPACK_RPM_PACKAGE_LICENSE "MPLv2.0")
    set(CPACK_STRIP_FILES TRUE)
    find_library(LM NAMES m)
    set(UNIX_LIBRARIES ${LM})
endif()

set(CPACK_PACKAGE_VENDOR        "Vlad Sterzhanov aka Quadrocube")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
include(CPack)

include(CTest)

include_directories(${CMAKE_SOURCE_DIR}/include)

add_subdirectory(src)
add_subdirectory(debug)
add_subdirectory(lua)
