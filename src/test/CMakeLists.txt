# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

include_directories(${CMAKE_CURRENT_BINARY_DIR})

enable_testing()
add_executable(test_basics ${SYMTSERIES_SRC} test_basics.c)
add_test(NAME test_basics WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} COMMAND test_basics)
