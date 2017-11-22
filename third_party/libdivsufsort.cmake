cmake_minimum_required(VERSION 3.1)

project(libdivsufsort-download NONE)

include(ExternalProject)
ExternalProject_Add(
        libdivsufsort

        GIT_REPOSITORY "https://github.com/y-256/libdivsufsort.git"
        GIT_TAG "master"

        UPDATE_COMMAND ""
        PATCH_COMMAND ""

        SOURCE_DIR "${CMAKE_BINARY_DIR}/third_party/libdivsufsort/build"
        CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DUSE_OPENMP:BOOL=ON -DBUILD_DIVSUFSORT64:BOOL=ON
                   -DBUILD_SHARED_LIBS:BOOL=OFF -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/third_party/libdivsufsort/install

        TEST_COMMAND ""
)