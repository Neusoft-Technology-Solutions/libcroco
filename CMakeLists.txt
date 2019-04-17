cmake_minimum_required(VERSION 2.8)

DefaultCompileFlags(EXTERNAL)


# Path to main configure script
set(LIBCROCO_CONFIGURE_SCRIPT "./configure.ac")

# Parse configure script
set(LIBCROCO_AC_INIT_REGEX
  "^AC_INIT\\(\\[([^]]+)\\],\\[([^]]+)\\],\\[([^]]+)\\],\\[([^]]+)\\]\\)$")
file(STRINGS "${LIBCROCO_CONFIGURE_SCRIPT}" LIBCROCO_AC_INIT_LINE
  LIMIT_COUNT 1 REGEX "^AC_INIT")
# Description
string(REGEX REPLACE        "${LIBCROCO_AC_INIT_REGEX}" "\\1"
    LIBCROCO_DESCRIPTION    "${LIBCROCO_AC_INIT_LINE}")
# Version
string(REGEX REPLACE        "${LIBCROCO_AC_INIT_REGEX}" "\\2"
    LIBCROCO_VERSION_STRING "${LIBCROCO_AC_INIT_LINE}")
# Contact
string(REGEX REPLACE        "${LIBCROCO_AC_INIT_REGEX}" "\\3"
    LIBCROCO_CONTACT        "${LIBCROCO_AC_INIT_LINE}")
# Parse version tweaks
set(LIBCROCO_VERSION_REGEX "^([0-9]+)\\.([0-9]+)\\.([0-9]+)-?(.*)$")
string(REGEX REPLACE     "${LIBCROCO_VERSION_REGEX}" "\\1"
  LIBCROCO_VERSION_MAJOR "${LIBCROCO_VERSION_STRING}")
string(REGEX REPLACE     "${LIBCROCO_VERSION_REGEX}" "\\2"
  LIBCROCO_VERSION_MINOR "${LIBCROCO_VERSION_STRING}")
string(REGEX REPLACE     "${LIBCROCO_VERSION_REGEX}" "\\3"
  LIBCROCO_VERSION_PATCH "${LIBCROCO_VERSION_STRING}")

# Package version
set(LIBCROCO_VERSION
  "${LIBCROCO_VERSION_MAJOR}.${LIBCROCO_VERSION_MINOR}.${LIBCROCO_VERSION_PATCH}")

set( LIBCROCO_VERSION_NUMBER 612)
set( G_DISABLE_CHECKS 0)
set( VERSION    ${LIBCROCO_VERSION})

configure_file(src/libcroco-config.h.in src/libcroco-config.h @ONLY)

configure_file(src/libcroco.h libcroco/libcroco.h @ONLY)

# --------------------------------------------------------------------------------
# Version of libcroco is in LIBCROCO_VERSION
if(COMMAND NaProvideLicenseInfo)
   get_filename_component(croco_ver_out "${CMAKE_CURRENT_BINARY_DIR}/libcroco.ver" ABSOLUTE)
   file(REMOVE ${croco_ver_out})
   file(WRITE ${croco_ver_out} "CROCO ${LIBCROCO_VERSION}\n")
   NaProvideLicenseInfo(croco ${croco_ver_out}
                                       ${CMAKE_CURRENT_SOURCE_DIR}/COPYING.LIB)
endif()

if (NOT GLIB_FOUND)
    message(FATAL_ERROR "libcroco: Need GLIB library to build")
endif()

if(NOT XML_FOUND)
    message(FATAL_ERROR "libcroco: Need XML library to build")
endif()

set(CROCO_HEADERS
    src/libcroco.h
    src/cr-additional-sel.h
    src/cr-attr-sel.h
    src/cr-cascade.h
    src/cr-declaration.h
    src/cr-doc-handler.h
    src/cr-enc-handler.h
    src/cr-input.h
    src/cr-num.h
    src/cr-om-parser.h
    src/cr-parser.h
    src/cr-pseudo.h
    src/cr-rgb.h
    src/cr-selector.h
    src/cr-simple-sel.h
    src/cr-statement.h
    src/cr-stylesheet.h
    src/cr-term.h
    src/cr-tknzr.h
    src/cr-token.h
    src/cr-utils.h
    src/cr-fonts.h
    src/cr-sel-eng.h
    src/cr-style.h
    src/cr-prop-list.h
    src/cr-parsing-location.h
    src/cr-string.h
    src/libcroco-config.h
)

set(CROCO_SOURCES
  src/cr-utils.c
  src/cr-input.c
  src/cr-enc-handler.c
  src/cr-num.c
  src/cr-rgb.c
  src/cr-token.c
  src/cr-tknzr.c
  src/cr-term.c
  src/cr-attr-sel.c
  src/cr-pseudo.c
  src/cr-additional-sel.c
  src/cr-simple-sel.c
  src/cr-selector.c
  src/cr-doc-handler.c
  src/cr-parser.c
  src/cr-declaration.c
  src/cr-statement.c
  src/cr-stylesheet.c
  src/cr-cascade.c
  src/cr-om-parser.c
  src/cr-style.c
  src/cr-sel-eng.c
  src/cr-fonts.c
  src/cr-prop-list.c
  src/cr-parsing-location.c
  src/cr-string.c
)

na_include_directories(BEFORE SYSTEM
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}/src)

include_directories(. intl src ${XML_INCLUDE_DIR})



add_definitions("-DCROCO_COMPILATION")

if(CROCO_LINKTYPE STREQUAL "SHARED")
  set(LIBRARY_TYPE SHARED)
endif()


GenerateProject(${CROCO_SOURCES})
add_library(croco ${LIBRARY_TYPE} ${CROCO_SOURCES})

set(CROCO_LIBS
        ${GLIB_LIBRARIES}
        ${XML_LIBRARIES}
)

target_link_libraries(croco ${CROCO_LIBS})

na_install_dll(croco)


# cmake package info for the poor. Export variables as defined by proper cmake find_package
set(CROCO_FOUND        TRUE                    PARENT_SCOPE)
set(CROCO_LIBRARIES    croco                    PARENT_SCOPE)
set(CROCO_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR}/src PARENT_SCOPE)


