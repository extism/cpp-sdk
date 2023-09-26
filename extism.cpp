#include "extism.hpp"

namespace extism {

std::string Manifest::json() const {
  Json::Value doc;
  Json::Value wasm;
  for (auto w : this->wasm) {
    wasm.append(w.json());
  }

  doc["wasm"] = wasm;

  if (!this->config.empty()) {
    Json::Value conf;

    for (auto k : this->config) {
      conf[k.first] = k.second;
    }
    doc["config"] = conf;
  }

  if (!this->allowedHosts.empty()) {
    Json::Value h;

    for (auto s : this->allowedHosts.value) {
      h.append(s);
    }
    doc["allowed_hosts"] = h;
  }

  if (!this->allowedPaths.empty()) {
    Json::Value h;
    for (auto k : this->allowedPaths.value) {
      h[k.first] = k.second;
    }
    doc["allowed_paths"] = h;
  }

  if (!this->timeoutMs.empty()) {
    doc["timeout_ms"] = Json::Value(this->timeoutMs.value);
  }

  Json::FastWriter writer;
  return writer.write(doc);
}

Json::Value Wasm::json() {

  Json::Value doc;

  if (this->source == WasmSourcePath) {
    doc["path"] = this->ref;
  } else if (this->source == WasmSourceURL) {
    doc["url"] = this->ref;
    doc["method"] = this->httpMethod;

    Json::Value h;
    for (auto k : this->httpHeaders) {
      h[k.first] = this->httpHeaders[k.second];
    }
    doc["headers"] = h;
  } else if (this->source == WasmSourceBytes) {
    throw Error("TODO: WasmSourceBytes");
  }

  if (!this->_hash.empty()) {
    doc["hash"] = this->_hash.value;
  }

  return doc;
}

}; // namespace extism
