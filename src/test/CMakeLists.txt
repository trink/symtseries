# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
set(STS_TEST_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/test.c
)
add_executable(test_private_sts ${SYMTSERIES_SRC} ${STS_TEST_SRC} test_private_sts.c)
add_test(NAME test_private_sts WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} COMMAND test_private_sts)
