# Installation

Build ssdeep and libfuzzy with CMake 3.20 or later, a C99 compiler, and a
C++11 compiler. The library-only build needs only a C compiler. Windows
builds use MinGW-w64, including its resource compiler.

## Build and install

From the source directory:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
cmake --install build
```

The default build produces the ssdeep command-line program and a shared
libfuzzy library. Installation includes the library, public headers, manual
page, and project documentation. Installing to a system directory may require
elevated permissions for the install command.

To choose another installation prefix, add `-DCMAKE_INSTALL_PREFIX=/path/to/prefix`
to the configure command. Packaging tools can stage an installation with
`DESTDIR=/path/to/staging cmake --install build` on Unix.

For generators that support multiple configurations, pass `--config Release`
to build, test, install, and packaging commands as appropriate.

## Build options

| CMake option | Default | Purpose |
| --- | --- | --- |
| `BUILD_SHARED_LIBS` | `ON` | Set to `OFF` to build a static libfuzzy. |
| `SSDEEP_LIBONLY` | `OFF` | Build only libfuzzy, without the command-line program or manual page. |
| `SSDEEP_BITPARALLEL_STRING_OPS` | `ON` | Set to `OFF` to disable bit-parallel string operations. |
| `BUILD_TESTING` | `ON` | Build and register tests with CTest. |
| `CMAKE_BUILD_TYPE` | unset | Choose `Debug`, `Release`, or another configuration for single-configuration generators. |

Use `-DBUILD_TESTING=OFF` for a library-only build without test executables.
Use `CC` and `CXX`, or `CMAKE_C_COMPILER` and `CMAKE_CXX_COMPILER`, to select
compilers during the initial configuration. Use a separate build directory
for each toolchain or configuration.

## Windows cross-compilation

With the i686 MinGW-w64 toolchain installed:

```sh
cmake -S . -B build-win -DCMAKE_TOOLCHAIN_FILE=cmake/mingw32.cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
cmake --build build-win --parallel
```

The supplied toolchain targets 32-bit Windows. For other targets, provide a
CMake toolchain file selecting the appropriate C, C++, and resource compilers.

## Packages

```sh
cmake --build build --target package
cmake --build build --target package_source
```

These commands create a ZIP binary package and a gzip-compressed source
archive in the build directory. The Windows binary package includes the
program and DLL when shared libraries are enabled. Use CPack's `-G` option
to select another available package format.

The helper scripts `normal.sh`, `win.sh`, and `world.sh` configure a native
debug build, a Windows release build, and Windows/source packages respectively.

## Clean

```sh
cmake --build build --target clean
```

Remove the build directory to discard the generated configuration as well.
