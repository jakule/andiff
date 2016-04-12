# - Try to find libdivsufsort
# Once done this will define
#  LIBDIVSUFSORT_FOUND - System has libdivsufsort
#  LIBDIVSUFSORT_INCLUDE_DIR - The libdivsufsort include directories
#  LIBDIVSUFSORT_LIBRARY - The libraries needed to use libdivsufsort
#  LIBDIVSUFSORT64_LIBRARY - The libraries needed to use libdivsufsort - 64bit version

find_path(LIBDIVSUFSORT_INCLUDE_DIR divsufsort.h
          HINTS ${LIBDIVSUFSORT_PREFIX} ${LIBDIVSUFSORT_INCLUDE_DIR}
          PATH_SUFFIXES include)

find_path(LIBDIVSUFSORT64_INCLUDE_DIR divsufsort64.h
          HINTS ${LIBDIVSUFSORT_PREFIX} ${LIBDIVSUFSORT64_INCLUDE_DIR}
          PATH_SUFFIXES include)

find_library(LIBDIVSUFSORT_LIBRARY NAMES divsufsort
             HINTS ${LIBDIVSUFSORT_PREFIX}  ${LIBDIVSUFSORT_LIBDIR}
             PATH_SUFFIXES lib)

find_library(LIBDIVSUFSORT64_LIBRARY NAMES divsufsort64
             HINTS ${LIBDIVSUFSORT_PREFIX}/lib  ${LIBDIVSUFSORT64_LIBDIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(libdivsufsort  DEFAULT_MSG
                                  LIBDIVSUFSORT_LIBRARY
                                  LIBDIVSUFSORT_INCLUDE_DIR)
find_package_handle_standard_args(libdivsufsort64 DEFAULT_MSG
                                  LIBDIVSUFSORT64_LIBRARY
                                  LIBDIVSUFSORT64_INCLUDE_DIR)

mark_as_advanced(LIBDIVSUFSORT_INCLUDE_DIR LIBDIVSUFSORT_LIBRARY LIBDIVSUFSORT64_INCLUDE_DIR LIBDIVSUFSORT64_LIBRARY)
