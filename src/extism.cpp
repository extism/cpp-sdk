#include "extism.hpp"

namespace extism {

// Set global log file for plugins
inline bool setLogFile(const char *filename, const char *level) {
  return extism_log_file(filename, level);
}

// Get libextism version
inline std::string_view version() { return extism_version(); }
}; // namespace extism