# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Build lua wrap library

project(sax C)
include(${CMAKE_MODULE_PATH}/dist.cmake)
add_definitions(-DLUA_COMPAT_MODULE -DDIST_VERSION="${DIST_VERSION}")
include(lua)

if(STS_LINK_STATIC)
    install_lua_module(sax lua_sax.c LINK symtseries_stat)
else()
    install_lua_module(sax lua_sax.c LINK symtseries)
endif()
add_lua_test(test.lua)
