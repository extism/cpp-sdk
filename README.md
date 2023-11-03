# Extism

C++ Host SDK for Extism

## Dependencies

- C++ compiler
- [libextism](https://extism.org/docs/install)
- cmake
  - Debian: `sudo apt install cmake`
  - macOS: `brew install cmake`
- jsoncpp
  - Debian: `sudo apt install libjsoncpp-dev`
  - macOS: `brew install jsoncpp`

If you wish to link libextism-cpp with its deps statically, to make a binary
with only system deps, instead of installing `jsoncpp` from your system
package manager, you must build and install it from source. This is needed to
get the static library `libjsoncpp.a`.

```shell
git clone https://github.com/open-source-parsers/jsoncpp
cd jsoncpp
cmake -B build && cmake --build build -j
make -C build install
```

## Building

```shell
$ mkdir build
$ cd build
$ cmake ..
$ make
```

## Testing

After building, run the following from the build directory:

```shell
$ make test
```

## Installation

After building, run the following from the build directory:

```shell
$ sudo make install
```

## Usage

After installing, `pkg-config` may be used to get needed linking flags.

### Dynamic
```shell
$ pkg-config --libs extism-cpp
```

### Static
```shell
$ pkg-config --static --libs extism-cpp-static
```
