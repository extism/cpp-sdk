#pragma once

#include <cstdint>
#include <extism.h>
#include <functional>
#include <json/json.h>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace extism {

class Error : public std::runtime_error {
public:
  Error(const std::string &msg) : std::runtime_error(msg) {}
  Error(const char *msg) : std::runtime_error(msg) {}
};

typedef std::map<std::string, std::string> Config;

template <typename T> class ManifestKey {
  bool is_set;

public:
  T value;
  ManifestKey(T x, bool is_set = false) : is_set(is_set), value(std::move(x)) {}
  ManifestKey() : is_set(false) {}

  void set(T x) {
    value = std::move(x);
    is_set = true;
  }

  bool empty() const { return !is_set; }
};

enum WasmSource { WasmSourcePath, WasmSourceURL, WasmSourceBytes };

class Wasm {
  WasmSource source;
  std::string ref;
  std::string httpMethod;
  std::map<std::string, std::string> httpHeaders;

  // TODO: add base64 encoded raw data
  ManifestKey<std::string> _hash;

public:
  Wasm(WasmSource source, std::string ref, std::string hash = std::string())
      : source(source), ref(std::move(ref)), _hash(std::move(hash)) {}

  // Create Wasm pointing to a path
  static Wasm path(std::string s, std::string hash = std::string()) {
    return Wasm(WasmSourcePath, std::move(s), std::move(hash));
  }

  // Create Wasm pointing to a URL
  static Wasm url(std::string s, std::string hash = std::string(),
                  std::string method = "GET",
                  std::map<std::string, std::string> headers =
                      std::map<std::string, std::string>()) {
    auto wasm = Wasm(WasmSourceURL, std::move(s), std::move(hash));
    wasm.httpMethod = std::move(method);
    wasm.httpHeaders = std::move(headers);
    return wasm;
  }

  Json::Value json() const;
};

class Manifest {
public:
  Config config;
  std::vector<Wasm> wasm;
  ManifestKey<std::vector<std::string>> allowedHosts;
  ManifestKey<std::map<std::string, std::string>> allowedPaths;
  ManifestKey<uint64_t> timeout;

  Manifest() : timeout(0, false) {}

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

  std::string string() const { return static_cast<std::string>(*this); }

  std::vector<uint8_t> vector() const {
    return static_cast<std::vector<uint8_t>>(*this);
  }

  operator std::string() const {
    return std::string(reinterpret_cast<const char *>(data), length);
  }
  operator std::vector<uint8_t>() const {
    return std::vector<uint8_t>(data, data + length);
  }
};

typedef ExtismValType ValType;
typedef ExtismValUnion ValUnion;
typedef ExtismVal Val;
typedef uint64_t MemoryHandle;

class CurrentPlugin {
  ExtismCurrentPlugin *const pointer;
  const ExtismVal *const inputs;
  const size_t nInputs;
  ExtismVal *const outputs;
  const size_t nOutputs;

public:
  CurrentPlugin(ExtismCurrentPlugin *p, const ExtismVal *inputs, size_t nInputs,
                ExtismVal *outputs, size_t nOutputs)
      : pointer(p), inputs(inputs), nInputs(nInputs), outputs(outputs),
        nOutputs(nOutputs) {}

  uint8_t *memory() const;
  uint8_t *memory(MemoryHandle offs) const;
  ExtismSize memoryLength(MemoryHandle offs) const;
  MemoryHandle memoryAlloc(ExtismSize size) const;
  void memoryFree(MemoryHandle handle) const;
  void output(const std::string &s, size_t index = 0) const;
  void output(const uint8_t *bytes, size_t len, size_t index = 0) const;
  void output(const Json::Value &&v, size_t index = 0) const;
  uint8_t *inputBytes(size_t *length = nullptr, size_t index = 0) const;
  Buffer inputBuffer(size_t index = 0) const;
  std::string inputString(size_t index = 0) const;
  const Val &inputVal(size_t index) const;
  Val &outputVal(size_t index) const;
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
  Function(std::string name, const std::vector<ValType> &inputs,
           const std::vector<ValType> &outputs, FunctionType f,
           void *userData = NULL, std::function<void(void *)> free = nullptr);

  Function(std::string ns, std::string name, const std::vector<ValType> &inputs,
           const std::vector<ValType> &outputs, FunctionType f,
           void *userData = NULL, std::function<void(void *)> free = nullptr);

  void setNamespace(std::string s) const;

  Function(const Function &f);

  ExtismFunction *get() const;
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

  // Reset the Extism runtime, this will invalidate all allocated memory
  // returns true if it succeeded
  bool reset() const;
};

// Set global log file for plugins
inline bool setLogFile(const char *filename, const char *level);

// Get libextism version
inline std::string version();
} // namespace extism
