#include "extism.hpp"

namespace extism {

// Set global log file for plugins
inline bool setLogFile(const char *filename, const char *level) {
  return extism_log_file(filename, level);
}

// Get libextism version
inline std::string version() { return std::string(extism_version()); }
}; // namespace extism