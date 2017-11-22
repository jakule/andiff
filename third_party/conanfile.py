from conans import ConanFile, CMake


class LibdivsufsortConan(ConanFile):
    name = "libdivsufsort"
    version = "2.0.1"
    license = "https://raw.githubusercontent.com/y-256/libdivsufsort/master/LICENSE"
    url = "<Package recipe repository url here, for issues about the package>"
    description = "libdivsufsort is a software library that implements a lightweight suffix array construction algorithm."
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "build64": [True, False], "use_openmp": [True, False]}
    default_options = "shared=False", "build64=False", "use_openmp=False"
    generators = "cmake"

    def source(self):
        self.run("git clone https://github.com/y-256/libdivsufsort.git")
        self.run("cd libdivsufsort && git checkout 2.0.1")

    def build(self):
        cmake = CMake(self)
        cmake.definitions["BUILD_DIVSUFSORT64"] = self.options.build64
        cmake.definitions["BUILD_SHARED_LIBS"] = self.options.shared
        cmake.definitions["USE_OPENMP"] = self.options.use_openmp

        cmake.configure(source_dir="%s/libdivsufsort" % self.source_folder)
        cmake.build()

    def package(self):
        self.copy("divsufsort*.h", src="include", keep_path=True)
        self.copy("*.a", src="lib", keep_path=True)

    def package_info(self):
        self.cpp_info.libs = ["libdivsufsort"]
