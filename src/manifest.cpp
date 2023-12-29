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
  return Wasm(WasmSourcePath, std::move(s), std::move(hash));
}

// Create Wasm pointing to a URL
Wasm Wasm::url(std::string s, std::string hash, std::string method,
               std::map<std::string, std::string> headers) {
  auto wasm = Wasm(WasmSourceURL, std::move(s), std::move(hash));
  wasm.httpMethod = std::move(method);
  wasm.httpHeaders = std::move(headers);
  return wasm;
}

// Create Wasm from bytes of a module
Wasm Wasm::bytes(const uint8_t *data, const size_t len, std::string hash) {
  std::string s = base64_encode(data, len);
  return Wasm(WasmSourceBytes, std::move(s), std::move(hash));
}

Wasm Wasm::bytes(const std::vector<uint8_t> &data, std::string hash) {
  return Wasm::bytes(data.data(), data.size(), hash);
}

class Serializer {
public:
  static Json::Value json(const Wasm &wasm) {
    Json::Value doc;

    if (wasm.source == WasmSourcePath) {
      doc["path"] = wasm.ref;
    } else if (wasm.source == WasmSourceURL) {
      doc["url"] = wasm.ref;
      doc["method"] = wasm.httpMethod;
      if (!wasm.httpHeaders.empty()) {
        Json::Value h;
        for (auto k : wasm.httpHeaders) {
          h[k.first] = k.second;
        }
        doc["headers"] = h;
      }
    } else if (wasm.source == WasmSourceBytes) {
      doc["data"] = wasm.ref;
    }

    if (!wasm._hash.empty()) {
      doc["hash"] = wasm._hash;
    }

    return doc;
  }
};

std::string Manifest::json() const {
  Json::Value doc;
  Json::Value wasm;
  for (auto w : this->wasm) {
    wasm.append(Serializer::json(w));
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

/*
Json::Value Wasm::json() const {

  Json::Value doc;

  if (this->source == WasmSourcePath) {
    doc["path"] = this->ref;
  } else if (this->source == WasmSourceURL) {
    doc["url"] = this->ref;
    doc["method"] = this->httpMethod;
    if (!this->httpHeaders.empty()) {
      Json::Value h;
      for (auto k : this->httpHeaders) {
        h[k.first] = k.second;
      }
      doc["headers"] = h;
    }
  } else if (this->source == WasmSourceBytes) {
    doc["data"] = this->ref;
  }

  if (!this->_hash.empty()) {
    doc["hash"] = this->_hash;
  }

  return doc;
}
*/

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

// Create manifest from Wasm data
Manifest Manifest::wasmBytes(const uint8_t *data, const size_t len,
                             std::string hash) {
  Manifest m;
  m.addWasmBytes(data, len, hash);
  return m;
}

Manifest Manifest::wasmBytes(const std::vector<uint8_t> &data,
                             std::string hash) {
  Manifest m;
  m.addWasmBytes(data, hash);
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

// add Wasm from bytes
void Manifest::addWasmBytes(const uint8_t *data, const size_t len,
                            std::string hash) {
  Wasm w = Wasm::bytes(data, len, hash);
  this->wasm.push_back(w);
}

void Manifest::addWasmBytes(const std::vector<uint8_t> &data,
                            std::string hash) {
  Wasm w = Wasm::bytes(data, hash);
  this->wasm.push_back(w);
}

// Add host to allowed hosts
void Manifest::allowHost(std::string host) {
  this->allowedHosts.push_back(host);
}

// Add path to allowed paths
void Manifest::allowPath(std::string src, std::string dest) {
  if (dest.empty()) {
    dest = src;
  }
  this->allowedPaths[src] = dest;
}

// Set timeout in milliseconds
void Manifest::setTimeout(uint64_t ms) { this->timeout = ms; }

// Set config key/value
void Manifest::setConfig(std::string k, std::string v) { this->config[k] = v; }

}; // namespace extism
