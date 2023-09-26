#pragma once

#include <cstring>
#include <functional>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <jsoncpp/json/json.h>

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
  ManifestKey<uint64_t> timeoutMs;

  // Empty manifest
  Manifest()
      : timeoutMs(0, false), allowedHosts(std::vector<std::string>(), false),
        allowedPaths(std::map<std::string, std::string>(), false) {}

  // Create manifest with a single Wasm from a path
  static Manifest wasmPath(std::string s, std::string hash = std::string()) {
    Manifest m;
    m.addWasmPath(s, hash);
    return m;
  }

  // Create manifest with a single Wasm from a URL
  static Manifest wasmURL(std::string s, std::string hash = std::string()) {
    Manifest m;
    m.addWasmURL(s, hash);
    return m;
  }

  std::string json() const;

  // Add Wasm from path
  void addWasmPath(std::string s, std::string hash = std::string()) {
    Wasm w = Wasm::path(s, hash);
    this->wasm.push_back(w);
  }

  // Add Wasm from URL
  void addWasmURL(std::string u, std::string hash = std::string()) {
    Wasm w = Wasm::url(u, hash);
    this->wasm.push_back(w);
  }

  // Add host to allowed hosts
  void allowHost(std::string host) {
    if (this->allowedHosts.empty()) {
      this->allowedHosts.set(std::vector<std::string>{});
    }
    this->allowedHosts.value.push_back(host);
  }

  // Add path to allowed paths
  void allowPath(std::string src, std::string dest = std::string()) {
    if (this->allowedPaths.empty()) {
      this->allowedPaths.set(std::map<std::string, std::string>{});
    }

    if (dest.empty()) {
      dest = src;
    }
    this->allowedPaths.value[src] = dest;
  }

  // Set timeout in milliseconds
  void setTimeout(uint64_t ms) { this->timeoutMs = ms; }

  // Set config key/value
  void setConfig(std::string k, std::string v) { this->config[k] = v; }
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

  uint8_t *memory() { return extism_current_plugin_memory(this->pointer); }
  uint8_t *memory(MemoryHandle offs) { return this->memory() + offs; }

  ExtismSize memoryLength(MemoryHandle offs) {
    return extism_current_plugin_memory_length(this->pointer, offs);
  }

  MemoryHandle memoryAlloc(ExtismSize size) {
    return extism_current_plugin_memory_alloc(this->pointer, size);
  }

  void memoryFree(MemoryHandle handle) {
    extism_current_plugin_memory_free(this->pointer, handle);
  }

  void outputString(const std::string &s, size_t index = 0) {
    this->outputBytes((const uint8_t *)s.c_str(), s.size(), index);
  }

  void outputBytes(const uint8_t *bytes, size_t len, size_t index = 0) {
    if (index < this->nInputs) {
      auto offs = this->memoryAlloc(len);
      memcpy(this->memory() + offs, bytes, len);
      this->outputs[index].v.i64 = offs;
    }
  }

  void outputJSON(const Json::Value &&v, size_t index = 0) {
    Json::FastWriter writer;
    const std::string s = writer.write(v);
    outputString(s, index);
  }

  uint8_t *inputBytes(size_t *length = nullptr, size_t index = 0) {
    if (index >= this->nInputs) {
      return nullptr;
    }
    auto inp = this->inputs[index];
    if (inp.t != ValType::I64) {
      return nullptr;
    }
    if (length != nullptr) {
      *length = this->memoryLength(inp.v.i64);
    }
    return this->memory() + inp.v.i64;
  }

  Buffer inputBuffer(size_t index = 0) {
    size_t length = 0;
    auto ptr = inputBytes(&length, index);
    return Buffer(ptr, length);
  }

  std::string inputString(size_t index = 0) {
    size_t length = 0;
    char *buf = (char *)this->inputBytes(&length, index);
    return std::string(buf, length);
  }

  const Val &inputVal(size_t index) {
    if (index >= nInputs) {
      throw Error("Input out of bounds");
    }

    return this->inputs[index];
  }

  Val &outputVal(size_t index) {
    if (index >= nOutputs) {
      throw Error("Output out of bounds");
    }

    return this->outputs[index];
  }
};

typedef std::function<void(CurrentPlugin, void *user_data)> FunctionType;

struct UserData {
  FunctionType func;
  void *userData = NULL;
  std::function<void(void *)> freeUserData;
};

static void functionCallback(ExtismCurrentPlugin *plugin,
                             const ExtismVal *inputs, ExtismSize n_inputs,
                             ExtismVal *outputs, ExtismSize n_outputs,
                             void *user_data) {
  UserData *data = (UserData *)user_data;
  data->func(CurrentPlugin(plugin, inputs, n_inputs, outputs, n_outputs),
             data->userData);
}

static void freeUserData(void *user_data) {
  UserData *data = (UserData *)user_data;
  if (data->userData != nullptr && data->freeUserData != nullptr) {
    data->freeUserData(data->userData);
  }
}

class Function {
  std::shared_ptr<ExtismFunction> func;
  std::string name;
  UserData userData;

public:
  Function(std::string name, const std::vector<ValType> inputs,
           const std::vector<ValType> outputs, FunctionType f,
           void *userData = NULL, std::function<void(void *)> free = nullptr)
      : name(name) {
    this->userData.func = f;
    this->userData.userData = userData;
    this->userData.freeUserData = free;
    auto ptr = extism_function_new(
        this->name.c_str(), inputs.data(), inputs.size(), outputs.data(),
        outputs.size(), functionCallback, &this->userData, freeUserData);
    this->func = std::shared_ptr<ExtismFunction>(ptr, extism_function_free);
  }

  void setNamespace(std::string s) {
    extism_function_set_namespace(this->func.get(), s.c_str());
  }

  Function(const Function &f) { this->func = f.func; }

  ExtismFunction *get() { return this->func.get(); }
};

class CancelHandle {
  const ExtismCancelHandle *handle;

public:
  CancelHandle(const ExtismCancelHandle *x) : handle(x){};
  bool cancel() { return extism_plugin_cancel(this->handle); }
};

class Plugin {
  std::vector<Function> functions;

public:
  ExtismPlugin *plugin;
  // Create a new plugin
  Plugin(const uint8_t *wasm, ExtismSize length, bool withWasi = false,
         std::vector<Function> functions = std::vector<Function>())
      : functions(functions) {
    std::vector<const ExtismFunction *> ptrs;
    for (auto i : this->functions) {
      ptrs.push_back(i.get());
    }

    char *errmsg = nullptr;
    this->plugin = extism_plugin_new(wasm, length, ptrs.data(), ptrs.size(),
                                     withWasi, &errmsg);
    if (this->plugin == nullptr) {
      std::string s(errmsg);
      extism_plugin_new_error_free(errmsg);
      throw Error(s);
    }
  }

  Plugin(const std::string &str, bool withWasi = false,
         std::vector<Function> functions = {})
      : Plugin((const uint8_t *)str.c_str(), str.size(), withWasi, functions) {}

  Plugin(const std::vector<uint8_t> &data, bool withWasi = false,
         std::vector<Function> functions = {})
      : Plugin(data.data(), data.size(), withWasi, functions) {}

  CancelHandle cancelHandle() {
    return CancelHandle(extism_plugin_cancel_handle(this->plugin));
  }

  // Create a new plugin from Manifest
  Plugin(const Manifest &manifest, bool withWasi = false,
         std::vector<Function> functions = {})
      : Plugin(manifest.json().c_str(), withWasi, functions) {}

  ~Plugin() {
    extism_plugin_free(this->plugin);
    this->plugin = nullptr;
  }

  void config(const Config &data) {
    Json::Value conf;

    for (auto k : data) {
      conf[k.first] = k.second;
    }

    Json::FastWriter writer;
    auto s = writer.write(conf);
    this->config(s);
  }

  void config(const char *json, size_t length) {
    bool b = extism_plugin_config(this->plugin, (const uint8_t *)json, length);
    if (!b) {
      const char *err = extism_plugin_error(this->plugin);
      throw Error(err == nullptr ? "Unable to update plugin config" : err);
    }
  }

  void config(const std::string &json) {
    this->config(json.c_str(), json.size());
  }

  // Call a plugin
  Buffer call(const std::string &func, const uint8_t *input,
              ExtismSize inputLength) const {
    int32_t rc =
        extism_plugin_call(this->plugin, func.c_str(), input, inputLength);
    if (rc != 0) {
      const char *error = extism_plugin_error(this->plugin);
      if (error == nullptr) {
        throw Error("extism_call failed");
      }

      throw Error(error);
    }

    ExtismSize length = extism_plugin_output_length(this->plugin);
    const uint8_t *ptr = extism_plugin_output_data(this->plugin);
    return Buffer(ptr, length);
  }

  // Call a plugin function with std::vector<uint8_t> input
  Buffer call(const std::string &func,
              const std::vector<uint8_t> &input) const {
    return this->call(func, input.data(), input.size());
  }

  // Call a plugin function with string input
  Buffer call(const std::string &func,
              const std::string &input = std::string()) const {
    return this->call(func, (const uint8_t *)input.c_str(), input.size());
  }

  // Returns true if the specified function exists
  bool functionExists(const std::string &func) const {
    return extism_plugin_function_exists(this->plugin, func.c_str());
  }
};

// Set global log file for plugins
inline bool setLogFile(const char *filename, const char *level) {
  return extism_log_file(filename, level);
}

// Get libextism version
inline std::string version() { return std::string(extism_version()); }
} // namespace extism
