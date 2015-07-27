# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

add_library(sts_test SHARED sts_test.c)
add_executable(test_private_sts test_private_sts.c)
target_link_libraries(test_private_sts symtseries sts_test m)
add_test(NAME test_private_sts WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} COMMAND test_private_sts)
