# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(SYMTSERIES_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/symtseries.c
)

set(ENV{SYMTSERIES_SRC} ${SYMTSERIES_SRC})

set(CMAKE_SKIP_BUILD_RPATH FALSE CACHE STRING "" FORCE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE CACHE STRING "" FORCE)
set(CPACK_RPM_PACKAGE_LICENSE "MPLv2.0")
set(CPACK_STRIP_FILES TRUE)

if (DEFINED STS_COMPILE_UNIT_TESTS)
    enable_testing()
    add_subdirectory(test)
else()
    add_library(symtseries SHARED ${SYMTSERIES_SRC})
    install(TARGETS symtseries DESTINATION bin)
endif()
