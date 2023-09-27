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

  if (!this->timeout.empty()) {
    doc["timeout_ms"] = Json::Value(this->timeout.value);
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

Manifest Manifest::wasmPath(std::string s, std::string hash) {
  Manifest m;
  m.addWasmPath(s, hash);
  return m;
}

// Create manifest with a single Wasm from a URL
Manifest Manifest::wasmURL(std::string s, std::string hash) {
  Manifest m;
  m.addWasmURL(s, hash);
  return m;
}

// Add Wasm from path
void Manifest::addWasmPath(std::string s, std::string hash) {
  Wasm w = Wasm::path(s, hash);
  this->wasm.push_back(w);
}

// Add Wasm from URL
void Manifest::addWasmURL(std::string u, std::string hash) {
  Wasm w = Wasm::url(u, hash);
  this->wasm.push_back(w);
}

// Add host to allowed hosts
void Manifest::allowHost(std::string host) {
  if (this->allowedHosts.empty()) {
    this->allowedHosts.set(std::vector<std::string>{});
  }
  this->allowedHosts.value.push_back(host);
}

// Add path to allowed paths
void Manifest::allowPath(std::string src, std::string dest) {
  if (this->allowedPaths.empty()) {
    this->allowedPaths.set(std::map<std::string, std::string>{});
  }

  if (dest.empty()) {
    dest = src;
  }
  this->allowedPaths.value[src] = dest;
}

// Set timeout in milliseconds
void Manifest::setTimeout(uint64_t ms) { this->timeout = ms; }

// Set config key/value
void Manifest::setConfig(std::string k, std::string v) { this->config[k] = v; }

}; // namespace extism
