# Extism cpp-sdk

The C++ SDK for integrating with the [Extism](https://extism.org/) runtime. Add this library in your host C++ applications to run Extism plugins.

## Building and Installation

### Install Dependencies

- [libextism](https://extism.org/docs/install)
- cmake and jsoncpp
  - Debian: `sudo apt install cmake jsoncpp`
  - macOS: `brew install cmake jsoncpp`

If you wish to do link jsoncpp statically, or do an in-tree build, See
[Alternative Dependency Strategies](#Alternative-Dependency-Strategies).

### Build and Install cpp-sdk

```bash
cmake -B build
cmake --build build -j
sudo cmake --install build
```

## Getting Started

To add the cpp-sdk to a CMake project:

```cmake
find_package(extism-cpp)
target_link_libraries(target_name extism-cpp)
```

### Loading a Plug-in

The primary concept in Extism is the [plug-in](https://extism.org/docs/concepts/plug-in). A plug-in is code module stored in a `.wasm` file.

Plug-in code can come from a file on disk, object storage or any number of places. Since you may not have one handy let's load a demo plug-in from the web. Let's
start by creating a main function that loads a plug-in:


```cpp
#include <extism.hpp>

int main(void) {
  const auto manifest =
      extism::Manifest::wasmURL("https://github.com/extism/plugins/releases/"
                                "latest/download/count_vowels.wasm");
  extism::Plugin plugin(manifest, true);
}
```

## Linking

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

## Testing

```bash
cmake --build build --target test
```
