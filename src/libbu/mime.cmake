#                      M I M E . C M A K E
# BRL-CAD
#
# Copyright (c) 2015-2020 United States Government as represented by
# the U.S. Army Research Laboratory.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following
# disclaimer in the documentation and/or other materials provided
# with the distribution.
#
# 3. The name of the author may not be used to endorse or promote
# products derived from this software without specific prior written
# permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
# OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
# GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
###
#
# Generate libbu's mime type enums and C functions from text inputs
#

# Set locations for final mime C source and header files
set(BU_MIME_C_FILE ${CMAKE_CURRENT_BINARY_DIR}/mime.c)
set(BU_MIME_TYPES_H_FILE ${CMAKE_BINARY_DIR}/include/bu/mime_types.h)

# Set locations for intermediate C source and header files
set(mime_types_h_file_tmp ${CMAKE_CURRENT_BINARY_DIR}/mime_types.h.tmp)
set(mime_c_file_tmp ${CMAKE_CURRENT_BINARY_DIR}/mime.c.tmp)

# Common comment header on our C source and header files
set(header "/**\n * Mime type definitions and mapping functions\n *\n")
set(header "${header} * DO NOT EDIT THIS FILE DIRECTLY.\n *\n")
set(header "${header} * Apply updates to mime.types, mime_cad.types, and mime.cmake\n *\n")
set(header "${header} * The file misc/mime.types should be kept in sync with:\n")
set(header "${header} * http://svn.apache.org/repos/asf/httpd/httpd/trunk/docs/conf/mime.types\n *\n")
set(header "${header} * The file misc/mime_cad.types holds additions to the standard mime\n")
set(header "${header} * types such as recognized geometry file formats. Any \"local\" mime\n")
set(header "${header} * type definitions not part of the standard should be added there.\n *\n")
set(header "${header} * The file src/libbu/mime.cmake is the code that generated this file.\n *\n")
set(header "${header} * This file is generated from public domain data and is in the public domain.\n */\n\n")

# Header to our C header file
set(h_contents "${header}")
set(h_contents "${h_contents}#ifndef BU_MIME_TYPES_H\n")
set(h_contents "${h_contents}#define BU_MIME_TYPES_H\n")
set(h_contents "${h_contents}\n#include \"common.h\"\n")
set(h_contents "${h_contents}#include \"bu/defines.h\"\n")
set(h_contents "${h_contents}\n__BEGIN_DECLS\n")

# Header to our C source file
set(c_contents "${header}")
set(c_contents "${c_contents}#include \"common.h\"\n")
set(c_contents "${c_contents}#include \"bu/mime.h\"\n")
set(c_contents "${c_contents}#include \"bu/file.h\"\n")
set(c_contents "${c_contents}#include \"bu/str.h\"\n")

# Begin processing mime types, read them in
file(READ ${BRLCAD_SOURCE_DIR}/misc/mime.types MIME_TYPES)
file(READ ${BRLCAD_SOURCE_DIR}/misc/mime_cad.types CAD_TYPES)

set(MIME_TYPES "${MIME_TYPES}\n${CAD_TYPES}")
string(REGEX REPLACE "\r?\n" ";" TYPES "${MIME_TYPES}")

set(ACTIVE_TYPES)
foreach(line ${TYPES})
  if(NOT ${line} MATCHES "[#]")
    set(elements)
    string(REPLACE " " ";" linescrub "${line}")
    string(REPLACE "\t" ";" linescrub "${linescrub}")
    foreach(e ${linescrub})
      set(elements ${elements} ${e})
    endforeach(e ${linescrub})
    list(GET elements 0 mtype)
    string(REGEX REPLACE "/.+" "" mgrp "${mtype}")
    string(REPLACE "-" "_DASH_" mgrp "${mgrp}")
    set(ACTIVE_TYPES ${ACTIVE_TYPES} ${mgrp})
  endif(NOT ${line} MATCHES "[#]")
endforeach(line ${TYPES})
list(REMOVE_DUPLICATES ACTIVE_TYPES)
list(SORT ACTIVE_TYPES)

foreach(line ${TYPES})
  if(NOT ${line} MATCHES "[#]")
    set(elements)
    string(REPLACE " " ";" linescrub "${line}")
    string(REPLACE "\t" ";" linescrub "${linescrub}")
    foreach(e ${linescrub})
      set(elements ${elements} ${e})
    endforeach(e ${linescrub})
    list(GET elements 0 mtype)
    list(REMOVE_AT elements 0)
    string(REGEX REPLACE "/.+" "" mime_group "${mtype}")
    string(REPLACE "-" "_DASH_" mime_group "${mime_group}")
    set(mime_type "${mtype}")
    string(TOUPPER "${mime_type}" mime_type)
    string(REPLACE "/" "_" mime_type "${mime_type}")
    string(REPLACE "." "_" mime_type "${mime_type}")
    string(REPLACE "+" "_PLUS_" mime_type "${mime_type}")
    string(REPLACE "-" "_DASH_" mime_type "${mime_type}")
    set(${mime_group}_types ${${mime_group}_types} ${mime_type})
    foreach(e ${elements})
      set(${mime_type}_extensions ${${mime_type}_extensions} ${e})
    endforeach(e ${elements})
  endif(NOT ${line} MATCHES "[#]")
endforeach(line ${TYPES})

# HEADER with typedefs

list(GET ACTIVE_TYPES 0 FIRST_TYPE)
set(mcstr "typedef enum {")
foreach(context ${ACTIVE_TYPES})
  string(TOUPPER "${context}" c)
  if("${context}" STREQUAL "${FIRST_TYPE}")
    set(mcstr "${mcstr}\n    BU_MIME_${c} = 0,")
  else("${context}" STREQUAL "${FIRST_TYPE}")
    set(mcstr "${mcstr}\n    BU_MIME_${c},")
  endif("${context}" STREQUAL "${FIRST_TYPE}")
endforeach(context ${ACTIVE_TYPES})
set(mcstr "${mcstr}\n    BU_MIME_UNKNOWN")
set(mcstr "${mcstr}\n} bu_mime_context_t;\n\n")
set(h_contents "${h_contents}\n${mcstr}")

foreach(c ${ACTIVE_TYPES})
  set(enum_str "typedef enum {")
  list(SORT ${c}_types)
  string(TOUPPER "${c}" c_u)
  # Default first enum for all types is an auto type
  set(enum_str "${enum_str}\n    BU_MIME_${c_u}_AUTO,")
  foreach(type ${${c}_types})
    set(enum_str "${enum_str}\n    BU_MIME_${type},")
  endforeach(type ${${c}_types})
  set(enum_str "${enum_str}\n    BU_MIME_${c_u}_UNKNOWN")
  set(enum_str "${enum_str}\n} bu_mime_${c}_t;\n\n")
  set(h_contents "${h_contents}\n${enum_str}")
endforeach(c ${ACTIVE_TYPES})
set(h_contents "${h_contents}\n__END_DECLS\n")
set(h_contents "${h_contents}\n#endif /*BU_MIME_TYPES_H*/\n")

# C mapping functions - extension to type
foreach(c ${ACTIVE_TYPES})
  set(enum_str "HIDDEN int\nmime_${c}(const char *ext)\n{")
  list(SORT ${c}_types)
  foreach(type ${${c}_types})
    foreach(ext ${${type}_extensions})
      set(enum_str "${enum_str}\n    if (BU_STR_EQUIV(ext, \"${ext}\"))\n\treturn ((int)BU_MIME_${type});\n")
    endforeach(ext ${${type}_extensions})
  endforeach(type ${${c}_types})
  set(enum_str "${enum_str}\n    return -1;\n}\n")
  set(c_contents "${c_contents}\n${enum_str}")
endforeach(c ${ACTIVE_TYPES})

# Public C mapping function - extension to type
set(mcstr "\nint\nbu_file_mime(const char *ext, bu_mime_context_t context)\n{")
set(mcstr "${mcstr}\n    switch (context) {\n")
foreach(context ${ACTIVE_TYPES})
  string(TOUPPER "${context}" c)
  set(mcstr "${mcstr}        case BU_MIME_${c}:\n")
  set(mcstr "${mcstr}             return mime_${context}(ext);\n")
  set(mcstr "${mcstr}             break;\n")
endforeach(context ${ACTIVE_TYPES})
set(mcstr "${mcstr}        default:\n")
set(mcstr "${mcstr}             return -1;\n")
set(mcstr "${mcstr}             break;\n")
set(mcstr "${mcstr}    }\n")
set(mcstr "${mcstr}    return -1;\n")
set(mcstr "${mcstr}}\n")
set(c_contents "${c_contents}\n${mcstr}")

# C mapping functions - type to string
foreach(c ${ACTIVE_TYPES})
  set(enum_str "HIDDEN const char *\nmime_str_${c}(int type)\n{")
  string(TOUPPER "${c}" c_u)
  set(enum_str "${enum_str}\n    const char *ret = NULL;")
  set(enum_str "${enum_str}\n    if (type < 0 || type >= BU_MIME_${c_u}_UNKNOWN) {\n\tret = bu_strdup(\"BU_MIME_${c_u}_UNKNOWN\");\n\tgoto found_type;\n    }\n")
  list(SORT ${c}_types)
  foreach(type ${${c}_types})
    set(enum_str "${enum_str}\n    if (type == (int)BU_MIME_${type}) {\n\tret = bu_strdup(\"BU_MIME_${type}\");\n\tgoto found_type;\n    }\n")
  endforeach(type ${${c}_types})
  set(enum_str "${enum_str}\n    found_type:\n")
  set(enum_str "${enum_str}\n    return ret;\n}\n")
  set(c_contents "${c_contents}\n${enum_str}")
endforeach(c ${ACTIVE_TYPES})

# Public C mapping function - type to string
set(mcstr "\nconst char *\nbu_file_mime_str(int t, bu_mime_context_t context)\n{")
set(mcstr "${mcstr}\n    switch (context) {\n")
foreach(context ${ACTIVE_TYPES})
  string(TOUPPER "${context}" c)
  set(mcstr "${mcstr}        case BU_MIME_${c}:\n")
  set(mcstr "${mcstr}             return mime_str_${context}(t);\n")
  set(mcstr "${mcstr}             break;\n")
endforeach(context ${ACTIVE_TYPES})
set(mcstr "${mcstr}        default:\n")
set(mcstr "${mcstr}             return NULL;\n")
set(mcstr "${mcstr}             break;\n")
set(mcstr "${mcstr}    }\n")
set(mcstr "${mcstr}    return NULL;\n")
set(mcstr "${mcstr}}\n")
set(c_contents "${c_contents}\n${mcstr}")


# Public C mapping function - string to type
set(enum_str "int\nbu_file_mime_int(const char *str)\n{")
foreach(c ${ACTIVE_TYPES})
  list(SORT ${c}_types)
  foreach(type ${${c}_types})
    set(enum_str "${enum_str}\n  if (BU_STR_EQUAL(str, \"BU_MIME_${type}\"))\n\treturn (int)BU_MIME_${type};\n")
  endforeach(type ${${c}_types})
endforeach(c ${ACTIVE_TYPES})
set(enum_str "${enum_str}\n  return -1;\n}\n")
set(c_contents "${c_contents}\n${enum_str}")

# C mapping functions - type to extension(s)
foreach(c ${ACTIVE_TYPES})
  set(enum_str "HIDDEN const char *\nmime_ext_${c}(int type)\n{")
  string(TOUPPER "${c}" c_u)
  set(enum_str "${enum_str}\n    const char *ret = NULL;")
  set(enum_str "${enum_str}\n    if (type < 0 || type >= BU_MIME_${c_u}_UNKNOWN) {\n\tgoto found_type;\n    }\n")
  list(SORT ${c}_types)
  foreach(type ${${c}_types})
    set(ext_str "${${type}_extensions}")
    string(REPLACE ";" "," ext_str "${ext_str}")
    set(enum_str "${enum_str}\n    if (type == (int)BU_MIME_${type}) {\n\tret = bu_strdup(\"${ext_str}\");\n\tgoto found_type;\n    }\n")
  endforeach(type ${${c}_types})
  set(enum_str "${enum_str}\n    found_type:\n")
  set(enum_str "${enum_str}\n    return ret;\n}\n")
  set(c_contents "${c_contents}\n${enum_str}")
endforeach(c ${ACTIVE_TYPES})

# Public C mapping function - type to string
set(mcstr "\nconst char *\nbu_file_mime_ext(int t, bu_mime_context_t context)\n{")
set(mcstr "${mcstr}\n    switch (context) {\n")
foreach(context ${ACTIVE_TYPES})
  string(TOUPPER "${context}" c)
  set(mcstr "${mcstr}        case BU_MIME_${c}:\n")
  set(mcstr "${mcstr}             return mime_ext_${context}(t);\n")
  set(mcstr "${mcstr}             break;\n")
endforeach(context ${ACTIVE_TYPES})
set(mcstr "${mcstr}        default:\n")
set(mcstr "${mcstr}             return NULL;\n")
set(mcstr "${mcstr}             break;\n")
set(mcstr "${mcstr}    }\n")
set(mcstr "${mcstr}    return NULL;\n")
set(mcstr "${mcstr}}\n")
set(c_contents "${c_contents}\n${mcstr}")

file(WRITE ${mime_types_h_file_tmp} "${h_contents}")
file(WRITE ${mime_c_file_tmp} "${c_contents}")

execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different ${mime_types_h_file_tmp} ${BU_MIME_TYPES_H_FILE})
execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different ${mime_c_file_tmp} ${BU_MIME_C_FILE})

execute_process(COMMAND ${CMAKE_COMMAND} -E remove ${mime_types_h_file_tmp})
execute_process(COMMAND ${CMAKE_COMMAND} -E remove ${mime_c_file_tmp})

DISTCLEAN(${BU_MIME_C_FILE} ${BU_MIME_TYPES_H_FILE})

# Local Variables:
# tab-width: 8
# mode: cmake
# indent-tabs-mode: t
# End:
# ex: shiftwidth=2 tabstop=8
