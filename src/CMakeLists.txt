# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

add_library(symtseries SHARED symtseries.c)
target_link_libraries(symtseries ${UNIX_LIBRARIES})

if (DEFINED STS_COMPILE_UNIT_TESTS)
    enable_testing()
    include_directories(test)
    add_subdirectory(test)
    target_link_libraries(symtseries sts_test)
endif()

install(TARGETS symtseries DESTINATION bin)
