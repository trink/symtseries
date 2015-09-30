# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project(symtseries)

set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Symbolic analysis for time series data")
set(CPACK_PACKAGE_VERSION_MAJOR 0)
set(CPACK_PACKAGE_VERSION_MINOR 1)

set(CPACK_RPM_PACKAGE_LICENSE "MPLv2.0")
set(CPACK_STRIP_FILES TRUE)
set(CPACK_PACKAGE_VENDOR        "Vlad Sterzhanov aka Quadrocube")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE.txt")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${CPACK_PACKAGE_VENDOR}")

set(CMAKE_SKIP_BUILD_RPATH              FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH      FALSE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH   FALSE)

if(MSVC)
    # Predefined Macros: http://msdn.microsoft.com/en-us/library/b0084kay.aspx
    # Compiler options: http://msdn.microsoft.com/en-us/library/fwkeyyhe.aspx
    # set a high warning level and treat them as errors
    set(CMAKE_C_FLAGS           "/W3 /WX")

    # debug complete debugging info, runtime error checking
    set(CMAKE_C_FLAGS_DEBUG     "/Zi /RTC1")

    # optimize for speed, auto inlining
    set(CMAKE_C_FLAGS_RELEASE   "/O2 /Ob2 /DNDEBUG")
else()
    # Compiler options: http://gcc.gnu.org/onlinedocs/gcc/Invoking-GCC.html#Invoking-GCC
    set(CMAKE_C_FLAGS   "-std=c99 -pedantic -Werror -Wall -Wextra")
    set(CMAKE_C_FLAGS_DEBUG     "-g")
    set(CMAKE_C_FLAGS_RELEASE   "-O2 -DNDEBUG")
    set(CMAKE_C_FLAGS_PROFILE   "${CMAKE_C_FLAGS_RELEASE} -g -pg")

    if(NOT WIN32)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
        find_library(LM NAMES m)
        set(UNIX_LIBRARIES ${LM})
    endif()
endif()

include(CPack)
include(CTest)

include_directories(${CMAKE_SOURCE_DIR}/include)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
find_package(Lua REQUIRED)

add_subdirectory(src)

if(LUA_FOUND)
    add_subdirectory(lua)
ENDIF(LUA_FOUND)
