# LuaDist CMake utility library.
# Provides sane project defaults and macros common to LuaDist CMake builds.
#
# Copyright (C) 2007-2012 LuaDist.
# by David Manura, Peter Drahoš
# Redistribution and use of this file is allowed according to the terms of the MIT license.
# For details see the COPYRIGHT file distributed with LuaDist.
# Please note that the package source code is licensed under its own license.

# Parser macro
macro ( parse_arguments prefix arg_names option_names)
  set ( DEFAULT_ARGS )
  foreach ( arg_name ${arg_names} )
    set ( ${prefix}_${arg_name} )
  endforeach ()
  foreach ( option ${option_names} )
    set ( ${prefix}_${option} FALSE )
  endforeach ()

  set ( current_arg_name DEFAULT_ARGS )
  set ( current_arg_list )
  foreach ( arg ${ARGN} )
    set ( larg_names ${arg_names} )
    list ( FIND larg_names "${arg}" is_arg_name )
    if ( is_arg_name GREATER -1 )
      set ( ${prefix}_${current_arg_name} ${current_arg_list} )
      set ( current_arg_name ${arg} )
      set ( current_arg_list )
    else ()
      set ( loption_names ${option_names} )
      list ( FIND loption_names "${arg}" is_option )
      if ( is_option GREATER -1 )
        set ( ${prefix}_${arg} TRUE )
      else ()
        set ( current_arg_list ${current_arg_list} ${arg} )
      endif ()
    endif ()
  endforeach ()
  set ( ${prefix}_${current_arg_name} ${current_arg_list} )
endmacro ()

