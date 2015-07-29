# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Build main library
add_library(symtseries SHARED symtseries.c)
target_link_libraries(symtseries ${UNIX_LIBRARIES})
install(TARGETS symtseries DESTINATION bin)

# Build public unit tests
add_subdirectory(test)

# Build private unit tests
include_directories(test)
add_executable(sts_test_private symtseries.c)
target_compile_definitions(sts_test_private PRIVATE -DSTS_COMPILE_UNIT_TESTS)
target_link_libraries(sts_test_private sts_test)
add_test(NAME sts_test_private WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/test COMMAND sts_test_private)
