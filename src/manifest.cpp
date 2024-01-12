#include "extism.hpp"
#include <algorithm>
#include <json/json.h>

namespace extism {

static std::string base64_encode(const uint8_t *data, size_t len) {
  const size_t out_len = ((len + 3 - 1) / 3) * 4;
  std::string out(out_len, '\0');
  static const char alpha[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                              "abcdefghijklmnopqrstuvwxyz"
                              "0123456789+/";

  char *out_cursor = out.data();
  while (len > 0) {
    const size_t to_encode = std::min<size_t>(3, len);
    len -= to_encode;
    uint8_t c[4];
    c[1] = c[2] = 0;
    memcpy(c, data, to_encode);
    data += to_encode;
    const uint32_t u =
        (uint32_t)c[0] << 16 | (uint32_t)c[1] << 8 | (uint32_t)c[2];
    *out_cursor++ = alpha[u >> 18];
    *out_cursor++ = alpha[u >> 12 & 63];
    *out_cursor++ = to_encode < 2 ? '=' : alpha[u >> 6 & 63];
    *out_cursor++ = to_encode < 3 ? '=' : alpha[u & 63];
  }
  return out;
}

// Create Wasm pointing to a path
Wasm Wasm::path(std::string s, std::string hash) {
  return Wasm(std::filesystem::path(std::move(s)), std::move(hash));
}

// Create Wasm pointing to a URL
Wasm Wasm::url(std::string s, std::string hash, std::string method,
               std::map<std::string, std::string> headers) {
  return Wasm(WasmURL(std::move(s), std::move(method), std::move(headers)),
              std::move(hash));
}

// Create Wasm from bytes of a module
Wasm Wasm::bytes(const uint8_t *data, const size_t len, std::string hash) {
  return Wasm(WasmBytes(data, len), std::move(hash));
}

Wasm Wasm::bytes(const std::vector<uint8_t> &data, std::string hash) {
  return Wasm::bytes(data.data(), data.size(), std::move(hash));
}

class Serializer {
public:
  static Json::Value json(const Wasm &wasm, const bool selfContained = true) {
    Json::Value doc;

    if (std::holds_alternative<std::filesystem::path>(wasm.src)) {
      doc["path"] = std::string(std::get<std::filesystem::path>(wasm.src));
    } else if (std::holds_alternative<WasmURL>(wasm.src)) {
      const auto &wasmURL = std::get<WasmURL>(wasm.src);
      doc["url"] = wasmURL.url;
      doc["method"] = wasmURL.httpMethod;
      if (!wasmURL.httpHeaders.empty()) {
        Json::Value h;
        for (auto k : wasmURL.httpHeaders) {
          h[k.first] = k.second;
        }
        doc["headers"] = h;
      }
    } else if (std::holds_alternative<WasmBytes>(wasm.src)) {
      const auto &wasmBytes = std::get<WasmBytes>(wasm.src);
      auto src = wasmBytes.get();
      auto srcSize = wasmBytes.getSize();
      if (selfContained) {
        doc["data"] = base64_encode(src, srcSize);
      } else {
        Json::Value data;
        data["ptr"] = reinterpret_cast<uint64_t>(src);
        data["len"] = reinterpret_cast<uint64_t>(srcSize);
        doc["data"] = data;
      }
    }

    if (!wasm._hash.empty()) {
      doc["hash"] = wasm._hash;
    }

    return doc;
  }
};

std::string Manifest::json(const bool selfContained) const {
  Json::Value doc;
  Json::Value wasm;
  for (const auto &w : this->wasm) {
    wasm.append(Serializer::json(w, selfContained));
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

    for (auto s : this->allowedHosts) {
      h.append(s);
    }
    doc["allowed_hosts"] = h;
  }

  if (!this->allowedPaths.empty()) {
    Json::Value h;
    for (auto k : this->allowedPaths) {
      h[k.first] = k.second;
    }
    doc["allowed_paths"] = h;
  }

  if (this->timeout.has_value()) {
    doc["timeout_ms"] = Json::Value(*this->timeout);
  }

  Json::FastWriter writer;
  return writer.write(doc);
}

Manifest Manifest::wasmPath(std::string s, std::string hash) {
  return Manifest({Wasm(std::filesystem::path(std::move(s)), std::move(hash))});
}

// Create manifest with a single Wasm from a URL
Manifest Manifest::wasmURL(std::string s, std::string hash) {
  return Manifest({Wasm(WasmURL(std::move(s)), std::move(hash))});
}

// Create manifest from Wasm data
Manifest Manifest::wasmBytes(const uint8_t *data, const size_t len,
                             std::string hash) {
  return Manifest({Wasm(WasmBytes(data, len), std::move(hash))});
}

Manifest Manifest::wasmBytes(const std::vector<uint8_t> &data,
                             std::string hash) {
  return Manifest::wasmBytes(data.data(), data.size(), std::move(hash));
}

// Add Wasm
void Manifest::addWasm(Wasm wasm) { this->wasm.push_back(std::move(wasm)); }

// Add Wasm from path
void Manifest::addWasmPath(std::string s, std::string hash) {
  Wasm w = Wasm::path(std::move(s), std::move(hash));
  this->wasm.push_back(std::move(w));
}

// Add Wasm from URL
void Manifest::addWasmURL(std::string u, std::string hash) {
  Wasm w = Wasm::url(std::move(u), std::move(hash));
  this->wasm.push_back(std::move(w));
}

// add Wasm from bytes
void Manifest::addWasmBytes(const uint8_t *data, const size_t len,
                            std::string hash) {
  Wasm w = Wasm::bytes(data, len, std::move(hash));
  this->wasm.push_back(std::move(w));
}

void Manifest::addWasmBytes(const std::vector<uint8_t> &data,
                            std::string hash) {
  Wasm w = Wasm::bytes(data, std::move(hash));
  this->wasm.push_back(std::move(w));
}

// Add host to allowed hosts
void Manifest::allowHost(std::string host) {
  this->allowedHosts.push_back(std::move(host));
}

// Add path to allowed paths
void Manifest::allowPath(std::string src, std::string dest) {
  if (dest.empty()) {
    dest = src;
  }
  this->allowedPaths[std::move(src)] = std::move(dest);
}

// Set timeout in milliseconds
void Manifest::setTimeout(uint64_t ms) { this->timeout = ms; }

// Set config key/value
void Manifest::setConfig(std::string k, std::string v) {
  this->config[std::move(k)] = std::move(v);
}

}; // namespace extism
