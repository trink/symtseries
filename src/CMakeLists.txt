# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(CMAKE_SKIP_BUILD_RPATH FALSE CACHE STRING "" FORCE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE CACHE STRING "" FORCE)
set(CPACK_RPM_PACKAGE_LICENSE "MPLv2.0")
set(CPACK_STRIP_FILES TRUE)

add_library(symtseries SHARED symtseries.c)

if (DEFINED STS_COMPILE_UNIT_TESTS)
    enable_testing()
    include_directories(test)
    add_subdirectory(test)
    target_link_libraries(symtseries sts_test)
endif()

install(TARGETS symtseries DESTINATION bin)
