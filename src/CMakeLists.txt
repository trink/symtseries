# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Build main library
add_library(symtseries SHARED symtseries.c)
target_link_libraries(symtseries ${UNIX_LIBRARIES})
install(TARGETS symtseries DESTINATION bin)

# Build private unit tests
include_directories(test)
add_executable(sts_test symtseries.c)
target_compile_definitions(sts_test PRIVATE -DSTS_COMPILE_UNIT_TESTS)
target_link_libraries(sts_test ${UNIX_LIBRARIES})
add_test(NAME sts_test COMMAND sts_test)
