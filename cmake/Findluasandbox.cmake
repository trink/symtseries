# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Look for the header file.
FIND_PATH(LUASANDBOX_INCLUDE_DIR NAMES luasandbox.h PATHS "/usr/local/include" "/usr/include")
MARK_AS_ADVANCED(LUASANDBOX_INCLUDE_DIR)

# Look for aux library
FIND_PATH(AUX_INCLUDE_DIR NAMES lauxlib.h PATHS "/usr/local/include" "/usr/include" ${LUASANDBOX_INCLUDE_DIR}/luasandbox)
MARK_AS_ADVANCED(AUX_INCLUDE_DIR)

# Look for the sandbox library.
FIND_LIBRARY(LUASANDBOX_LIBRARY NAMES
    luasandbox
    PATHS "/usr/local/lib" "/usr/lib"
    )
MARK_AS_ADVANCED(LUASANDBOX_LIBRARY)

# Look for the Lua library.
FIND_LIBRARY(LUASB_LIBRARY NAMES
    luasb
    PATHS "/usr/local/lib" "/usr/lib"
    )
MARK_AS_ADVANCED(LUASB_LIBRARY)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LUASANDBOX LUASANDBOX_LIBRARY LUASB_LIBRARY AUX_INCLUDE_DIR LUASANDBOX_INCLUDE_DIR)

IF(LUASANDBOX_FOUND)
  SET(LUASANDBOX_LIBRARIES ${LUASANDBOX_LIBRARY} ${LUASB_LIBRARY})
  SET(LUASANDBOX_INCLUDE_DIRS ${LUASANDBOX_INCLUDE_DIR} ${AUX_INCLUDE_DIR})
ENDIF(LUASANDBOX_FOUND)
