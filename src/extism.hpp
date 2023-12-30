#pragma once

#include <cstdint>
#include <extism.h>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace extism {

class Error : public std::runtime_error {
public:
  Error(const std::string &msg) : std::runtime_error(msg) {}
  Error(const char *msg) : std::runtime_error(msg) {}
};

typedef std::map<std::string, std::string> Config;

enum WasmSource { WasmSourcePath, WasmSourceURL, WasmSourceBytes };

class Wasm {
  WasmSource source;
  std::string ref;
  std::string httpMethod;
  std::map<std::string, std::string> httpHeaders;

  // TODO: add base64 encoded raw data
  std::string _hash;

public:
  Wasm(WasmSource source, std::string ref, std::string hash = std::string())
      : source(source), ref(std::move(ref)), _hash(std::move(hash)) {}

  // Create Wasm pointing to a path
  static Wasm path(std::string s, std::string hash = std::string());

  // Create Wasm pointing to a URL
  static Wasm url(std::string s, std::string hash = std::string(),
                  std::string method = "GET",
                  std::map<std::string, std::string> headers = {});

  // Create Wasm from bytes of a module
  static Wasm bytes(const uint8_t *data, const size_t len,
                    std::string hash = std::string());

  // Create Wasm from bytes of a module
  static Wasm bytes(const std::vector<uint8_t> &data,
                    std::string hash = std::string());

  friend class Serializer;
};

class Manifest {
public:
  Config config;
  std::vector<Wasm> wasm;
  std::vector<std::string> allowedHosts;
  std::map<std::string, std::string> allowedPaths;
  std::optional<uint64_t> timeout;

  Manifest() {}

  // Create manifest with a single Wasm from a path
  static Manifest wasmPath(std::string s, std::string hash = std::string());

  // Create manifest with a single Wasm from a URL
  static Manifest wasmURL(std::string s, std::string hash = std::string());

  // Create manifest from Wasm data
  static Manifest wasmBytes(const uint8_t *data, const size_t len,
                            std::string hash = std::string());
  static Manifest wasmBytes(const std::vector<uint8_t> &data, std::string hash);

  std::string json() const;

  // Add Wasm from path
  void addWasmPath(std::string s, std::string hash = std::string());

  // Add Wasm from URL
  void addWasmURL(std::string u, std::string hash = std::string());

  // add Wasm from bytes
  void addWasmBytes(const uint8_t *data, const size_t len,
                    std::string hash = std::string());
  void addWasmBytes(const std::vector<uint8_t> &data, std::string hash);

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
  Buffer(const uint8_t *ptr, size_t len) : data(ptr), length(len) {}
  const uint8_t *const data;
  const size_t length;

  std::string_view string() const {
    return static_cast<std::string_view>(*this);
  }

  std::vector<uint8_t> vector() const {
    return static_cast<std::vector<uint8_t>>(*this);
  }

  operator std::string_view() const {
    return std::string_view(reinterpret_cast<const char *>(data), length);
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
  bool output(std::string_view s, size_t index = 0) const;
  bool output(const uint8_t *bytes, size_t len, size_t index = 0) const;
  uint8_t *inputBytes(size_t *length = nullptr, size_t index = 0) const;
  Buffer inputBuffer(size_t index = 0) const;
  std::string_view inputStringView(size_t index = 0) const;
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

  Function(const std::string &ns, std::string name,
           const std::vector<ValType> &inputs,
           const std::vector<ValType> &outputs, FunctionType f,
           void *userData = NULL, std::function<void(void *)> free = nullptr);

  void setNamespace(const std::string &s) const;

  Function(const Function &f);

  ExtismFunction *get() const;
};

class Plugin {
  std::vector<Function> functions;

  struct PluginDeleter {
    void operator()(ExtismPlugin *) const;
  };
  using unique_plugin = std::unique_ptr<ExtismPlugin, PluginDeleter>;
  unique_plugin plugin;

public:
  class CancelHandle {
    const ExtismCancelHandle *handle;

  public:
    CancelHandle(const ExtismCancelHandle *x) : handle(x){};
    bool cancel();
  };

  // Create a new plugin
  Plugin(const uint8_t *wasm, size_t length, bool withWasi = false,
         std::vector<Function> functions = std::vector<Function>());

  Plugin(std::string_view str, bool withWasi = false,
         std::vector<Function> functions = {});

  Plugin(const std::vector<uint8_t> &data, bool withWasi = false,
         std::vector<Function> functions = {});

  CancelHandle cancelHandle();

  // Create a new plugin from Manifest
  Plugin(const Manifest &manifest, bool withWasi = false,
         std::vector<Function> functions = {});

  void config(const Config &data);

  void config(const char *json, size_t length);

  void config(std::string_view json);

  // Call a plugin
  Buffer call(const char *func, const uint8_t *input, size_t inputLength) const;

  // Call a plugin function with std::vector<uint8_t> input
  Buffer call(const char *func, const std::vector<uint8_t> &input) const;

  // Call a plugin function with string input
  Buffer call(const char *func, std::string_view input = "") const;

  // Call a plugin
  Buffer call(const std::string &func, const uint8_t *input,
              size_t inputLength) const;

  // Call a plugin function with std::vector<uint8_t> input
  Buffer call(const std::string &func, const std::vector<uint8_t> &input) const;

  // Call a plugin function with string input
  Buffer call(const std::string &func, std::string_view input = "") const;

  // Returns true if the specified function exists
  bool functionExists(const char *func) const;

  // Returns true if the specified function exists
  bool functionExists(const std::string &func) const;

  // Reset the Extism runtime, this will invalidate all allocated memory
  // returns true if it succeeded
  bool reset() const;

  // Get a ptr to the plugin that can be passed to the c api
  ExtismPlugin *get() const { return plugin.get(); }
};

// Set global log file for plugins
inline bool setLogFile(const char *filename, const char *level);

// Get libextism version
inline std::string_view version();
} // namespace extism
