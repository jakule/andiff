Andiff
======
[![Build Status](https://travis-ci.org/jakule/andiff.svg?branch=master)](https://travis-ci.org/jakule/andiff)

Andiff is another version of bsdiff application focused on performance. Original application can be found [here](http://www.daemonology.net/bsdiff/).

Prerequisites
=============

* C++11 compiler - tested with GCC 4.8+ and Clang 3.8+
* CMake 3.1.3: [https://cmake.org/](https://cmake.org/)

The dependencies below can be installed with [Conan](https://www.conan.io/):
* bzip2 library: [http://www.bzip.org/](http://www.bzip.org/)
* libdivsufsort: [https://github.com/y-256/libdivsufsort](https://github.com/y-256/libdivsufsort)

**Ubuntu 14.04:**

```shell
sudo apt-get install build-essential
```

> **Note:** For Ubuntu 14.04 required CMake is not available in Ubuntu repository and should be downloaded from CMake [site](https://cmake.org/download/).

**Fedora 22:**
```shell
sudo dnf install gcc-c++ cmake
```

Building with Conan
===================

For details how to install Conan look here: [https://www.conan.io/](https://www.conan.io/).
When you've got Conan installed, just follow the instructions:

```shell
git clone https://github.com/jakule/andiff.git
cd andiff
mkdir build && cd build/
# The repository below contains packages not available in official Conan repository.
conan remote add bintray-jakule https://api.bintray.com/conan/jakule/public-conan
conan install .. -s build_type=Release --build=missing
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . 
ctest -V
# Optional to create tar.gz package
cmake --build . --target package
```

Manual building process
=======================

Install missing dependencies:

**Ubuntu 14.04:**

```shell
sudo apt-get install libbz2-dev
```

**Fedora 22:**
```shell
sudo dnf install bzip2-libs
```

#### libdivsufsort

> **Note:** It is strongly recommended to compile libdivsufsort with OpenMP support.

```shell
cmake -G"Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DUSE_OPENMP:BOOL=ON \
-DBUILD_DIVSUFSORT64:BOOL=ON -DBUILD_SHARED_LIBS:BOOL=OFF \
-DCMAKE_INSTALL_PREFIX:PATH=${LIBDIVSUFSORT_INSTALL_PREFIX} ../libdivsufsort/
cmake --build .
cmake --build . --target install
```

#### andiff

```shell
cmake -G"Unix Makefiles" -DCMAKE_BUILD_TYPE=Release \
-DLIBDIVSUFSORT_PREFIX:PATH=${LIBDIVSUFSORT_INSTALL_PREFIX} ../andiff
cmake --build .
```

## CMake compilation options:

* `ENABLE_ADDRESS_SANITIZER` - Enable Address Sanitizer; Default: OFF   
* `ENABLE_THREAD_SANITIZER` - Enable Thread Sanitizer; Default: OFF   
* `GENERATE_DWARF` - Generate DWARF debug symbols with Debug build; Default: OFF
* `ENABLE_NATIVE` - Add -march=native to compiler for Release build; Default:ON   

> **Warning:** If you want to use andiff on different machine than was compiled disable this option. 
Otherwise you may end up with *Illegal instruction* exception.

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
