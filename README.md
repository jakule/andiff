Andiff
======
[![Build Status](https://travis-ci.org/jakule/andiff.svg?branch=master)](https://travis-ci.org/jakule/andiff)

Andiff is another version of bsdiff application focused on performance. Original application can be found [here](http://www.daemonology.net/bsdiff/).

Prerequisites
=============

* C++11 compiler - tested with GCC 4.8, GCC 5.3 and Clang 3.8
* bzip2 library: [http://www.bzip.org/](http://www.bzip.org/)
* CMake 3.1: [https://cmake.org/](https://cmake.org/)
* libdivsufsort: [https://github.com/y-256/libdivsufsort](https://github.com/y-256/libdivsufsort)

Building
========

**Ubuntu 14.04:**

```shell
sudo apt-get install build-essential libbz2-dev
```

> **Note:** For Ubuntu 14.04 required CMake is not available in Ubuntu repository and should be downloaded from CMake [site](https://cmake.org/download/).

**Fedora 22:**
```shell
sudo dnf install gcc-c++ bzip2-libs cmake
```

#### libdivsufsort

> **Note:** It is strongly recommended to compile libdivsufsort with OpenMP support.

```shell
cmake -G"Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DUSE_OPENMP:BOOL=ON \
-DBUILD_DIVSUFSORT64:BOOL=ON -DBUILD_SHARED_LIBS:BOOL=OFF \
-DCMAKE_INSTALL_PREFIX:PATH=${LIBDIVSUFSORT_INSTALL_PREFIX} ../libdivsufsort/
make
make install
```

#### andiff

```shell
cmake -G"Unix Makefiles" -DCMAKE_BUILD_TYPE=Release \
-DLIBDIVSUFSORT_PREFIX:PATH=${LIBDIVSUFSORT_INSTALL_PREFIX} ../andiff
make
```

## CMake compilation options:

* `ENABLE_ADDRESS_SANITIZER` - Enable Address Sanitizer; Default: OFF   
* `ENABLE_THREAD_SANITIZER` - Enable Thread Sanitizer; Default: OFF   
* `GENERATE_DWARF` - Generate DWARF debug symbols with Debug build; Default: OFF
* `ENABLE_NATIVE` - Add -march=native to compiler for Release build; Default:ON   

> **Warning:** If you want to use andiff on different machine than was compiler disable this option. Other wise you may end with *Illegal instruction* exception.

Usage
=====

Generating patch:

```shell
./andiff oldfile newfile patchfile
```

Applying patch:

```shell
./anpatch odlfile newfile patchfile
```
