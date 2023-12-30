#include "extism.hpp"
#include <json/json.h>

namespace extism {

void extism::Plugin::PluginDeleter::operator()(ExtismPlugin *plugin) const {
  extism_plugin_free(plugin);
}

Plugin::Plugin(const uint8_t *wasm, size_t length, bool withWasi,
               std::vector<Function> functions)
    : functions(std::move(functions)) {
  std::vector<const ExtismFunction *> ptrs;
  for (auto i : this->functions) {
    ptrs.push_back(i.get());
  }

  char *errmsg = nullptr;
  this->plugin = unique_plugin(extism_plugin_new(
      wasm, length, ptrs.data(), ptrs.size(), withWasi, &errmsg));
  if (this->plugin == nullptr) {
    std::string s(errmsg);
    extism_plugin_new_error_free(errmsg);
    throw Error(s);
  }
}

Plugin::Plugin(std::string_view str, bool withWasi,
               std::vector<Function> functions)
    : Plugin(reinterpret_cast<const uint8_t *>(str.data()), str.size(),
             withWasi, std::move(functions)) {}

Plugin::Plugin(const std::vector<uint8_t> &data, bool withWasi,
               std::vector<Function> functions)
    : Plugin(data.data(), data.size(), withWasi, std::move(functions)) {}

Plugin::CancelHandle Plugin::cancelHandle() {
  return CancelHandle(extism_plugin_cancel_handle(this->plugin.get()));
}

// Create a new plugin from Manifest
Plugin::Plugin(const Manifest &manifest, bool withWasi,
               std::vector<Function> functions)
    : Plugin(manifest.json(), withWasi, std::move(functions)) {}

bool Plugin::CancelHandle::cancel() {
  return extism_plugin_cancel(this->handle);
}

void Plugin::config(const Config &data) {
  Json::Value conf;

  for (auto k : data) {
    conf[k.first] = k.second;
  }

  Json::FastWriter writer;
  auto s = writer.write(conf);
  this->config(s);
}

void Plugin::config(const char *json, size_t length) {
  bool b = extism_plugin_config(
      this->plugin.get(), reinterpret_cast<const uint8_t *>(json), length);
  if (!b) {
    const char *err = extism_plugin_error(this->plugin.get());
    throw Error(err == nullptr ? "Unable to update plugin config" : err);
  }
}

void Plugin::config(std::string_view json) {
  this->config(json.data(), json.size());
}

// Call a plugin
Buffer Plugin::call(const char *func, const uint8_t *input,
                    size_t inputLength) const {
  int32_t rc = extism_plugin_call(this->plugin.get(), func, input, inputLength);
  if (rc != 0) {
    const char *error = extism_plugin_error(this->plugin.get());
    if (error == nullptr) {
      throw Error("extism_call failed");
    }

    throw Error(error);
  }

  ExtismSize length = extism_plugin_output_length(this->plugin.get());
  const uint8_t *ptr = extism_plugin_output_data(this->plugin.get());
  return Buffer(ptr, length);
}

// Call a plugin function with std::vector<uint8_t> input
Buffer Plugin::call(const char *func, const std::vector<uint8_t> &input) const {
  return this->call(func, input.data(), input.size());
}

// Call a plugin function with string input
Buffer Plugin::call(const char *func, std::string_view input) const {
  return this->call(func, reinterpret_cast<const uint8_t *>(input.data()),
                    input.size());
}

// Call a plugin
Buffer Plugin::call(const std::string &func, const uint8_t *input,
                    size_t inputLength) const {
  return this->call(func.c_str(), input, inputLength);
}

// Call a plugin function with std::vector<uint8_t> input
Buffer Plugin::call(const std::string &func,
                    const std::vector<uint8_t> &input) const {
  return this->call(func.c_str(), input);
}

// Call a plugin function with string input
Buffer Plugin::call(const std::string &func, std::string_view input) const {
  return this->call(func.c_str(), input);
}

// Returns true if the specified function exists
bool Plugin::functionExists(const char *func) const {
  return extism_plugin_function_exists(this->plugin.get(), func);
}

// Returns true if the specified function exists
bool Plugin::functionExists(const std::string &func) const {
  return extism_plugin_function_exists(this->plugin.get(), func.c_str());
}

// Reset the Extism runtime, this will invalidate all allocated memory
// returns true if it succeeded
bool Plugin::reset() const { return extism_plugin_reset(this->plugin.get()); }

}; // namespace extism
