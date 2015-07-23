# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(CMAKE_SKIP_BUILD_RPATH FALSE CACHE STRING "" FORCE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE CACHE STRING "" FORCE)
set(CPACK_RPM_PACKAGE_LICENSE "MPLv2.0")
set(CPACK_STRIP_FILES TRUE)

remove_definitions(-DSTS_COMPILE_UNIT_TESTS)

set(DEBUG_LIBS
    ${CMAKE_CURRENT_SOURCE_DIR}/debug.h    
)

add_executable(sax_convert $ENV{SYMTSERIES_SRC} ${DEBUG_LIBS} sax_convert.c)
add_executable(fetch_data $ENV{SYMTSERIES_SRC} ${DEBUG_LIBS} fetch_data.c)

install(TARGETS sax_convert DESTINATION bin)
