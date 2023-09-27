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

