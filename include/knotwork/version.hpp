#pragma once

#include <string>

namespace knotwork {

struct VersionInfo {
  int major = 0;
  int minor = 1;
  int patch = 0;
  std::string name = "Knotwork";
};

[[nodiscard]] VersionInfo version_info();
[[nodiscard]] std::string version_string();
[[nodiscard]] std::string build_capabilities();

}  // namespace knotwork
