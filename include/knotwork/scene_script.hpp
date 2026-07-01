#pragma once

#include "knotwork/builder.hpp"

#include <string_view>

namespace knotwork {

struct ScriptIssue {
  std::uint32_t line = 0;
  std::string message;
};

struct ScriptResult {
  Scene scene;
  std::vector<ScriptIssue> issues;
};

[[nodiscard]] ScriptResult scene_from_script(std::string_view script);
[[nodiscard]] std::string scene_to_script(const Scene& scene);
[[nodiscard]] bool script_has_errors(const ScriptResult& result);

}  // namespace knotwork
