# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Build lua wrap library

project (sax C)
include (${CMAKE_MODULE_PATH}/dist.cmake)
include(lua)

install_lua_module (sax lua_sax.c)
target_link_libraries(sax symtseries)
