# Extism cpp-sdk

The C++ SDK for integrating with the [Extism](https://extism.org/) runtime. Add this library in your host C++ applications to run Extism plugins.

Join the [Extism Discord](https://discord.gg/UsNqcTTa9P) and chat with us!

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
target_link_libraries(getting-started extism-cpp)
```

### Loading a Plug-in

The primary concept in Extism is the [plug-in](https://extism.org/docs/concepts/plug-in). A plug-in is a code module stored in a `.wasm` file.

Plug-in code can come from a file on disk, object storage or any number of places. Since you may not have one handy, let's load a demo plug-in from the web. Let's
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

### Calling A Plug-in's Exports

This plug-in was written in Rust and it does one thing, it counts vowels in a string. It exposes one "export" function: `count_vowels`. We can call exports using `Plugin::call`.
Let's add code to call `count_vowels` to our main func:

```c++
#include <extism.hpp>
#include <iostream>
#include <string>

int main(void) {
  // ...

  const std::string hello("Hello, World!");
  auto out = plugin.call("count_vowels", hello);
  std::string response(out.string());
  std::cout << response << std::endl;
  // => {"count":3,"total":3,"vowels":"aeiouAEIOU"}
}
```

Build it:

```bash
cmake -B build && cmake --build build
```

Running this should print out the JSON vowel count report:

```shell
./build/getting-started
{"count":3,"total":3,"vowels":"aeiouAEIOU"}
```

All exports have the same interface, optional bytes in and optional bytes out. This plug-in happens to take a string and return a JSON encoded string with a report of results.

### Plug-in State

Plug-ins may be stateful or stateless. Plug-ins can maintain state between calls using variables. Our count vowels plug-in remembers the total number of counted vowels in the "total" key in the result. You can see this by making subsequent calls to the export:

```cpp
  const std::string hello("Hello, World!");
  auto out = plugin.call("count_vowels", hello);
  std::string response(out.string());
  std::cout << response << std::endl;
  // => {"count":3,"total":3,"vowels":"aeiouAEIOU"}

  out = plugin.call("count_vowels", hello);
  response = out.string();
  std::cout << response << std::endl;
  // => {"count":3,"total":6,"vowels":"aeiouAEIOU"}
```

The state variables will persist until the plug-in is freed or reinitialized.

### Configuration

Plug-ins may optionally take a configuration object. This is a static way to configure the plug-in. Our count-vowels plugin takes an optional configuration to change out which characters are considered vowels. Example:

```cpp
// #include ...

int main(void) {
  auto manifest =
      extism::Manifest::wasmURL("https://github.com/extism/plugins/releases/"
                                "latest/download/count_vowels.wasm");
  manifest.setConfig("vowels", "aeiouyAEIOUY");
  extism::Plugin plugin(manifest, true);

  const std::string hello("Yellow, World!");
  auto out = plugin.call("count_vowels", hello);
  std::string response(out.string());
  std::cout << response << std::endl;
  // => {"count":4,"total":4,"vowels":"aeiouyAEIOUY"}
}
```


### Host Functions

Let's extend our count-vowels example a little bit: Instead of storing the `total` in an ephemeral plug-in var, let's store it in a persistent key-value store!

Wasm can't use our KV store on it's own. This is where [Host Functions](https://extism.org/docs/concepts/host-functions) come in.

[Host functions](https://extism.org/docs/concepts/host-functions) allow us to grant new capabilities to our plug-ins from our application. In this case, they are native C++ functions that are passed to and can can be invoked by the plug-in.

Let's load the manifest like usual but load up the `count_vowels_kvstore` plug-in:

```cpp
  const auto manifest =
      extism::Manifest::wasmURL("https://github.com/extism/plugins/releases/"
                                "latest/download/count_vowels_kvstore.wasm");
```

> *Note*: The source code for this is [here](https://github.com/extism/plugins/blob/main/count_vowels_kvstore/src/lib.rs) and is written in rust, but it could be written in any of our PDK languages.

Unlike our previous plug-in, this plug-in expects you to provide host functions that satisfy its import interface for a KV store.

We want to expose two functions to our plugin, `kv_write` which writes a bytes value to a key and `kv_read` which reads the bytes at the given key:

```cpp
  // pretend this is Redis or something :)
  std::map<std::string, std::vector<uint8_t>> kvStore;

  auto t = std::vector<extism::ValType>{extism::ValType::I64};
  auto kvRead = extism::Function(
      "kv_read", t, t,
      [&kvStore](extism::CurrentPlugin plugin, void *user_data) {
        const auto it = kvStore.find(plugin.inputString());
        if (it == kvStore.end()) {
          const std::vector<uint8_t> zeros{0, 0, 0, 0};
          plugin.output(zeros.data(), zeros.size());
          return;
        }
        plugin.output(it->second.data(), it->second.size());
      });
  auto kvWrite = extism::Function(
      "kv_write", {extism::ValType::I64, extism::ValType::I64}, {},
      [&kvStore](extism::CurrentPlugin plugin, void *user_data) {
        const auto key = plugin.inputString(0);
        auto value = plugin.inputBuffer(1);
        kvStore[key] = value.vector();
      });
```

We need to pass these imports to the plug-in to create them. All imports of a plug-in must be satisfied for it to be initialized:

```cpp
  extism::Plugin plugin(manifest, true, {kvRead, kvWrite});
```

Now, we can demo it:

```cpp
  const std::string hello("Hello, World!");
  auto out = plugin.call("count_vowels", hello);
  std::string response(out.string());
  std::cout << response << std::endl;
  // => {"count":3,"total":3,"vowels":"aeiouAEIOU"}

  out = plugin.call("count_vowels", hello);
  response = out.string();
  std::cout << response << std::endl;
  // => {"count":3,"total":6,"vowels":"aeiouAEIOU"}
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
