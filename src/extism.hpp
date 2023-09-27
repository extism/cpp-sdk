#pragma once

#include <cstring>
#include <functional>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#if __has_include(<json/json.h>)
#include <json/json.h>
#else
#include <jsoncpp/json/json.h>
#endif

extern "C" {
#include <extism.h>
}

namespace extism {

class Error : public std::runtime_error {
public:
  Error(std::string msg) : std::runtime_error(msg) {}
};

typedef std::map<std::string, std::string> Config;

template <typename T> class ManifestKey {
  bool is_set = false;

public:
  T value;
  ManifestKey(T x, bool is_set = false) : is_set(is_set) { value = x; }

  void set(T x) {
    value = x;
    is_set = true;
  }

  bool empty() const { return is_set == false; }
};

enum WasmSource { WasmSourcePath, WasmSourceURL, WasmSourceBytes };

class Wasm {
  WasmSource source;
  std::string ref;
  std::string httpMethod;
  std::map<std::string, std::string> httpHeaders;

  // TODO: add base64 encoded raw data
  ManifestKey<std::string> _hash =
      ManifestKey<std::string>(std::string(), false);

public:
  Wasm(WasmSource source, std::string ref, std::string hash = std::string())
      : source(source), ref(ref), _hash(hash) {}

  // Create Wasm pointing to a path
  static Wasm path(std::string s, std::string hash = std::string()) {
    return Wasm(WasmSourcePath, s, hash);
  }

  // Create Wasm pointing to a URL
  static Wasm url(std::string s, std::string hash = std::string(),
                  std::string method = "GET",
                  std::map<std::string, std::string> headers =
                      std::map<std::string, std::string>()) {
    auto wasm = Wasm(WasmSourceURL, s, hash);
    wasm.httpMethod = method;
    wasm.httpHeaders = headers;
    return wasm;
  }

  Json::Value json();
};

class Manifest {
public:
  Config config;
  std::vector<Wasm> wasm;
  ManifestKey<std::vector<std::string>> allowedHosts;
  ManifestKey<std::map<std::string, std::string>> allowedPaths;
  ManifestKey<uint64_t> timeout;

  // Empty manifest
  Manifest()
      : timeout(0, false), allowedHosts(std::vector<std::string>(), false),
        allowedPaths(std::map<std::string, std::string>(), false) {}

  // Create manifest with a single Wasm from a path
  static Manifest wasmPath(std::string s, std::string hash = std::string());

  // Create manifest with a single Wasm from a URL
  static Manifest wasmURL(std::string s, std::string hash = std::string());

  std::string json() const;

  // Add Wasm from path
  void addWasmPath(std::string s, std::string hash = std::string());

  // Add Wasm from URL
  void addWasmURL(std::string u, std::string hash = std::string());

  // Add host to allowed hosts
  void allowHost(std::string host);

  // Add path to allowed paths
  void allowPath(std::string src, std::string dest = std::string());

  // Set timeout in milliseconds
  void setTimeout(uint64_t ms);

  // Set config key/value
  void setConfig(std::string k, std::string v);
};

class Buffer {
public:
  Buffer(const uint8_t *ptr, ExtismSize len) : data(ptr), length(len) {}
  const uint8_t *data;
  ExtismSize length;

  std::string string() { return (std::string)(*this); }

  std::vector<uint8_t> vector() { return (std::vector<uint8_t>)(*this); }

  operator std::string() { return std::string((const char *)data, length); }
  operator std::vector<uint8_t>() {
    return std::vector<uint8_t>(data, data + length);
  }
};

typedef ExtismValType ValType;
typedef ExtismValUnion ValUnion;
typedef ExtismVal Val;
typedef uint64_t MemoryHandle;

class CurrentPlugin {
  ExtismCurrentPlugin *pointer;
  const ExtismVal *inputs;
  size_t nInputs;
  ExtismVal *outputs;
  size_t nOutputs;

public:
  CurrentPlugin(ExtismCurrentPlugin *p, const ExtismVal *inputs, size_t nInputs,
                ExtismVal *outputs, size_t nOutputs)
      : pointer(p), inputs(inputs), nInputs(nInputs), outputs(outputs),
        nOutputs(nOutputs) {}

  uint8_t *memory();
  uint8_t *memory(MemoryHandle offs);
  ExtismSize memoryLength(MemoryHandle offs);
  MemoryHandle memoryAlloc(ExtismSize size);
  void memoryFree(MemoryHandle handle);
  void output(const std::string &s, size_t index = 0);
  void output(const uint8_t *bytes, size_t len, size_t index = 0);
  void output(const Json::Value &&v, size_t index = 0);
  uint8_t *inputBytes(size_t *length = nullptr, size_t index = 0);
  Buffer inputBuffer(size_t index = 0);
  std::string inputString(size_t index = 0);
  const Val &inputVal(size_t index);
  Val &outputVal(size_t index);
};

typedef std::function<void(CurrentPlugin, void *user_data)> FunctionType;

class Function {
public:
  struct UserData {
    FunctionType func;
    void *userData = NULL;
    std::function<void(void *)> freeUserData;
  };

private:
  std::shared_ptr<ExtismFunction> func;
  std::string name;
  UserData userData;

public:
  Function(std::string name, const std::vector<ValType> inputs,
           const std::vector<ValType> outputs, FunctionType f,
           void *userData = NULL, std::function<void(void *)> free = nullptr);

  void setNamespace(std::string s);

  Function(const Function &f);

  ExtismFunction *get();
};

class Plugin {
  std::vector<Function> functions;

public:
  class CancelHandle {
    const ExtismCancelHandle *handle;

  public:
    CancelHandle(const ExtismCancelHandle *x) : handle(x){};
    bool cancel() { return extism_plugin_cancel(this->handle); }
  };

  ExtismPlugin *plugin;
  // Create a new plugin
  Plugin(const uint8_t *wasm, ExtismSize length, bool withWasi = false,
         std::vector<Function> functions = std::vector<Function>());

  Plugin(const std::string &str, bool withWasi = false,
         std::vector<Function> functions = {});

  Plugin(const std::vector<uint8_t> &data, bool withWasi = false,
         std::vector<Function> functions = {});

  CancelHandle cancelHandle();

  // Create a new plugin from Manifest
  Plugin(const Manifest &manifest, bool withWasi = false,
         std::vector<Function> functions = {});

  ~Plugin();

  void config(const Config &data);

  void config(const char *json, size_t length);

  void config(const std::string &json);

  // Call a plugin
  Buffer call(const std::string &func, const uint8_t *input,
              ExtismSize inputLength) const;

  // Call a plugin function with std::vector<uint8_t> input
  Buffer call(const std::string &func, const std::vector<uint8_t> &input) const;

  // Call a plugin function with string input
  Buffer call(const std::string &func,
              const std::string &input = std::string()) const;

  // Returns true if the specified function exists
  bool functionExists(const std::string &func) const;
};

// Set global log file for plugins
inline bool setLogFile(const char *filename, const char *level);

// Get libextism version
inline std::string version();
} // namespace extism
