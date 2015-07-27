# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(CMAKE_SKIP_BUILD_RPATH FALSE CACHE STRING "" FORCE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE CACHE STRING "" FORCE)
set(CPACK_RPM_PACKAGE_LICENSE "MPLv2.0")
set(CPACK_STRIP_FILES TRUE)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(sax_convert sax_convert.c)
target_link_libraries(sax_convert symtseries m)
add_executable(fetch_data fetch_data.c)
target_link_libraries(fetch_data symtseries m)
