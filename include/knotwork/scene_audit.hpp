#pragma once

#include "knotwork/scene_inspector.hpp"

namespace knotwork {

struct AuditRule {
  std::string name;
  std::string description;
};

struct AuditFinding {
  std::string rule;
  Severity severity = Severity::Info;
  std::string message;
  std::uint32_t node = kNoIndex;
};

struct AuditOptions {
  std::uint32_t max_depth = 32;
  std::uint32_t max_children_per_node = 128;
  bool require_camera = true;
  bool require_light = false;
  bool require_unique_names = true;
};

[[nodiscard]] std::vector<AuditRule> default_audit_rules();
[[nodiscard]] std::vector<AuditFinding> audit_scene(const Scene& scene,
                                                    const AssetLibrary& assets,
                                                    AuditOptions options = {});
[[nodiscard]] std::string audit_to_text(const Scene& scene, std::span<const AuditFinding> findings);

}  // namespace knotwork
