
#include "extism.hpp"
#include <cstring>

namespace extism {

uint8_t *CurrentPlugin::memory() const {
  return extism_current_plugin_memory(this->pointer);
}
uint8_t *CurrentPlugin::memory(MemoryHandle offs) const {
  return this->memory() + offs;
}

ExtismSize CurrentPlugin::memoryLength(MemoryHandle offs) const {
  return extism_current_plugin_memory_length(this->pointer, offs);
}

MemoryHandle CurrentPlugin::memoryAlloc(ExtismSize size) const {
  return extism_current_plugin_memory_alloc(this->pointer, size);
}

void CurrentPlugin::memoryFree(MemoryHandle handle) const {
  extism_current_plugin_memory_free(this->pointer, handle);
}

void CurrentPlugin::output(const std::string &s, size_t index) const {
  this->output((const uint8_t *)s.c_str(), s.size(), index);
}

void CurrentPlugin::output(const uint8_t *bytes, size_t len,
                           size_t index) const {
  if (index < this->nOutputs) {
    auto offs = this->memoryAlloc(len);
    memcpy(this->memory() + offs, bytes, len);
    this->outputs[index].v.i64 = offs;
  }
}

uint8_t *CurrentPlugin::inputBytes(size_t *length, size_t index) const {
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

Buffer CurrentPlugin::inputBuffer(size_t index) const {
  size_t length = 0;
  auto ptr = inputBytes(&length, index);
  return Buffer(ptr, length);
}

std::string CurrentPlugin::inputString(size_t index) const {
  size_t length = 0;
  char *buf = (char *)this->inputBytes(&length, index);
  return std::string(buf, length);
}

const Val &CurrentPlugin::inputVal(size_t index) const {
  if (index >= nInputs) {
    throw Error("Input out of bounds");
  }

  return this->inputs[index];
}

Val &CurrentPlugin::outputVal(size_t index) const {
  if (index >= nOutputs) {
    throw Error("Output out of bounds");
  }

  return this->outputs[index];
}

}; // namespace extism
