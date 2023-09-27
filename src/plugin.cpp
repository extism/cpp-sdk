#include "extism.hpp"

namespace extism {

Plugin::Plugin(const uint8_t *wasm, ExtismSize length, bool withWasi,
               std::vector<Function> functions)
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

Plugin::Plugin(const std::string &str, bool withWasi,
               std::vector<Function> functions)
    : Plugin((const uint8_t *)str.c_str(), str.size(), withWasi, functions) {}

Plugin::Plugin(const std::vector<uint8_t> &data, bool withWasi,
               std::vector<Function> functions)
    : Plugin(data.data(), data.size(), withWasi, functions) {}

Plugin::CancelHandle Plugin::cancelHandle() {
  return CancelHandle(extism_plugin_cancel_handle(this->plugin));
}

// Create a new plugin from Manifest
Plugin::Plugin(const Manifest &manifest, bool withWasi,
               std::vector<Function> functions)
    : Plugin(manifest.json().c_str(), withWasi, functions) {}

Plugin::~Plugin() {
  if (this->plugin == nullptr)
    return;
  extism_plugin_free(this->plugin);
  this->plugin = nullptr;
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
  bool b = extism_plugin_config(this->plugin, (const uint8_t *)json, length);
  if (!b) {
    const char *err = extism_plugin_error(this->plugin);
    throw Error(err == nullptr ? "Unable to update plugin config" : err);
  }
}

void Plugin::config(const std::string &json) {
  this->config(json.c_str(), json.size());
}

// Call a plugin
Buffer Plugin::call(const std::string &func, const uint8_t *input,
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
Buffer Plugin::call(const std::string &func,
                    const std::vector<uint8_t> &input) const {
  return this->call(func, input.data(), input.size());
}

// Call a plugin function with string input
Buffer Plugin::call(const std::string &func, const std::string &input) const {
  return this->call(func, (const uint8_t *)input.c_str(), input.size());
}

// Returns true if the specified function exists
bool Plugin::functionExists(const std::string &func) const {
  return extism_plugin_function_exists(this->plugin, func.c_str());
}
}; // namespace extism
