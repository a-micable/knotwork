#pragma once

#include "knotwork/scene_audit.hpp"

namespace knotwork {

struct ValidationFinding {
  Severity severity = Severity::Info;
  SceneErrorCode code = SceneErrorCode::None;
  std::string path;
  std::string message;
};

struct ValidationReport {
  std::vector<ValidationFinding> findings;
  [[nodiscard]] bool ok() const;
};

[[nodiscard]] ValidationReport validate_scene_detailed(const Scene& scene);
[[nodiscard]] std::string validation_report_text(const ValidationReport& report);
[[nodiscard]] std::vector<ValidationFinding> reference_findings(const Scene& scene);
[[nodiscard]] std::vector<ValidationFinding> naming_findings(const Scene& scene);
[[nodiscard]] std::vector<ValidationFinding> transform_findings(const Scene& scene);

}  // namespace knotwork
