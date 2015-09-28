# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Build lua wrap library

include(${CMAKE_MODULE_PATH}/dist.cmake)
add_definitions(-DLUA_COMPAT_MODULE -DDIST_VERSION="0.1.0")
include(lua)

install_lua_module(sax lua_sax.c LINK symtseries_stat)

add_lua_test(test.lua)
