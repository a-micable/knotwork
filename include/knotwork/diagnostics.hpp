#pragma once

#include "knotwork/format.hpp"

#include <span>
#include <string>
#include <vector>

namespace knotwork {

enum class Severity {
  Info,
  Warning,
  Error,
};

struct Diagnostic {
  Severity severity = Severity::Info;
  SceneErrorCode code = SceneErrorCode::None;
  std::string message;
  std::uint32_t node = kNoIndex;
};

[[nodiscard]] const char* severity_name(Severity severity);
[[nodiscard]] std::vector<Diagnostic> diagnose_scene(const Scene& scene);
[[nodiscard]] std::string format_diagnostic(const Scene& scene, const Diagnostic& diagnostic);
[[nodiscard]] std::string format_diagnostics(const Scene& scene, std::span<const Diagnostic> diagnostics);

}  // namespace knotwork
