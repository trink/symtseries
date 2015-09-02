# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

include_directories(${LUASANDBOX_INCLUDE_DIRS})

# Build lua wrap library
add_library(luasax SHARED lua_sax.c)
target_link_libraries(luasax symtseries ${LUASANDBOX_LIBRARIES})
install(TARGETS luasax DESTINATION bin)
