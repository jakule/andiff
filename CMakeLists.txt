cmake_minimum_required(VERSION 3.1)
cmake_policy(SET CMP0048 NEW)

project(andiff VERSION 0.92)
message(STATUS "Project version: ${PROJECT_VERSION}")

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_GNUCC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fstrict-aliasing -Wunused-result")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g3")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -fno-omit-frame-pointer")
elseif(MSVC)
    if(MSVC_VERSION LESS 1900) # if MSVC < 14 2015
        add_compile_options("/D_ALLOW_KEYWORD_MACROS")      # Allow for hacks below
        add_compile_options("/Dnoexcept=throw()")           # Visual Studio 2013 doesn't understand noexcept
        add_compile_options("/Dconstexpr=")                 # Visual Studio 2013 doesn't understand constexpr
    endif()
    add_compile_options("/wd4244")                      # conversion from 'uint64_t' to 'unsigned int', possible loss of data
    add_compile_options(/D_CRT_SECURE_NO_WARNINGS)      # Eliminate deprecation warnings for the older, less secure functions.
    add_compile_options(/D_CRT_NONSTDC_NO_DEPRECATE)    # Disable warning "The POSIX name for this item is deprecated. Instead, use the ISO C++ conformant name: _open. See online help for details."
    add_compile_options(/DWIN32_LEAN_AND_MEAN)          # Remove a lot of garbage from windows headers.
    add_compile_options(/DNOMINMAX)                     # Remove definitions of min and max macros from windows headers.
endif()

option(ENABLE_ADDRESS_SANITIZER "Enable Address Sanitizer" OFF)
option(ENABLE_THREAD_SANITIZER "Enable Thread Sanitizer" OFF)
option(GENERATE_DWARF "Generate DWARF debug symbols" OFF)
option(ENABLE_NATIVE "Add -march=native to compiler for Release build" ON)

if(ENABLE_ADDRESS_SANITIZER)
    message(STATUS "Enabled ASAN")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
    set(CMAKE_CXX_LINKER_FLAGS "${CMAKE_CXX_LINKER_FLAGS} -fsanitize=address")
endif()

if(ENABLE_THREAD_SANITIZER)
    message(STATUS "Enabled TSAN")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread")
    set(CMAKE_CXX_LINKER_FLAGS "${CMAKE_CXX_LINKER_FLAGS} -fsanitize=thread")
endif()

if(GENERATE_DWARF)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -gdwarf-4  -gsplit-dwarf -fuse-ld=gold ")
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -Wl,--gdb-index")
endif()

if(ENABLE_NATIVE)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=native")
endif()

find_package(Threads REQUIRED)
find_package(BZip2 REQUIRED)
find_package(libdivsufsort REQUIRED)
find_package(Doxygen)

if(DOXYGEN_FOUND)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
    add_custom_target(doc
        ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating Doxygen documentation" VERBATIM)
endif(DOXYGEN_FOUND)

# TODO libdivsufsort requires OpenMP with static libraries
# and only when was compiled with OpenMP support
find_package(OpenMP)

set(DIFF_EXE_NAME "andiff")
set(PATCH_EXE_NAME "anpatch")

add_subdirectory(src)

enable_testing()
add_test(NAME SanityCheck
         COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests/sanity_check.py
                   --diff $<TARGET_FILE:${DIFF_EXE_NAME}>
                   --patch $<TARGET_FILE:${PATCH_EXE_NAME}>
                   --size 1)

