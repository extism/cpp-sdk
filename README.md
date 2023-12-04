# Extism cpp-sdk

C++ Host SDK for Extism

## Getting Started

### Install Dependencies

- [libextism](https://extism.org/docs/install)
- cmake and jsoncpp
  - Debian: `sudo apt install cmake jsoncpp`
  - macOS: `brew install cmake jsoncpp`

If you wish to do link jsoncpp statically, or do an in-tree build, See
[Alternative Dependency Strategies](#Alternative-Dependency-Strategies).

### Building

```bash
cmake -B build
cmake --build build -j
```

### Testing

After building, run the following from the build directory:

```bash
make test
```

### Installation

After building, run the following from the build directory:

```bash
sudo make install
```

### Linking

#### CMake

```cmake
find_package(extism-cpp)
target_link_libraries(target_name extism-cpp)
```

or statically:

```cmake
find_package(extism-cpp)
target_link_libraries(target_name extism-cpp-static)
```

#### `pkg-config`

```bash
pkg-config --libs extism-cpp
```

or statically:

```bash
pkg-config --static --libs extism-cpp-static
```

## Alternative Dependency Strategies

### link jsoncpp statically

For builds using `extism-cpp-static` to be static other than glibc, etc. jsoncpp
must be linked statically. jsoncpp provided by package managers often isn't
provided as a static library.

Building and installing jsoncpp from source (including `jsoncpp_static`):

```bash
git clone https://github.com/open-source-parsers/jsoncpp
cd jsoncpp
cmake -B build && cmake --build build -j
make -C build install
```

### In-Tree builds

If you wish, instead of using installed deps, you can do an in-tree build!

For example:

```bash
mkdir extism-cpp-project
cd extism-cpp-project
git init
git submodule add https://github.com/extism/extism.git
git submodule add https://github.com/open-source-parsers/jsoncpp.git
git submodule add https://github.com/extism/cpp-sdk.git
cd cpp-sdk
cmake -DEXTISM_CPP_BUILD_IN_TREE=1 -B build && cmake --build build
```

### Fallback Dependencies

If `EXTISM_CPP_BUILD_IN_TREE` is not set and `find_package` fails, the dependency is fetched from the internet using `FetchContent` and built from source.

The author believes this is the worst way to deal with the deps as it requires building them from source and doesn't use a centrally managed, flat tree of dependencies. It's provided just as a cross-platform convenience.
