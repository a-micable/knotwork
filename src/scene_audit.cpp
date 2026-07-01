#include "knotwork/scene_audit.hpp"
#include "knotwork/validate.hpp"

#include <map>
#include <sstream>

namespace knotwork {

std::vector<AuditRule> default_audit_rules() {
  return {
      {"validation", "scene must pass structural validation"},
      {"max-depth", "hierarchy depth should remain manageable"},
      {"fanout", "individual nodes should not own too many direct children"},
      {"camera", "deliverable scenes should include a camera"},
      {"light", "renderable scenes may require at least one light"},
      {"unique-names", "node names should be unique enough for tooling"},
      {"assets", "mesh nodes should resolve to mesh assets"},
  };
}

std::vector<AuditFinding> audit_scene(const Scene& scene, const AssetLibrary& assets, AuditOptions options) {
  std::vector<AuditFinding> findings;
  const SceneError validation = validate_scene(scene);
  if (validation.code != SceneErrorCode::None) {
    findings.push_back({"validation", Severity::Error, validation.message, kNoIndex});
  }
  const GraphStats stats = graph_stats(scene);
  if (stats.max_depth > options.max_depth) {
    findings.push_back({"max-depth", Severity::Warning, "scene hierarchy is deeper than configured limit", kNoIndex});
  }
  if (options.require_camera && stats.camera_count == 0) {
    findings.push_back({"camera", Severity::Warning, "scene has no camera", kNoIndex});
  }
  if (options.require_light && stats.light_count == 0) {
    findings.push_back({"light", Severity::Warning, "scene has no light", kNoIndex});
  }
  for (std::uint32_t i = 0; i < scene.nodes.size(); ++i) {
    const auto children = child_nodes(scene, i);
    if (children.size() > options.max_children_per_node) {
      findings.push_back({"fanout", Severity::Warning, "node has too many children", i});
    }
    const Node& node = scene.nodes[i];
    if ((node.kind == NodeKind::Mesh || node.kind == NodeKind::Instance) && node.material != kNoIndex &&
        assets.get(MeshHandle{node.material}) == nullptr) {
      findings.push_back({"assets", Severity::Warning, "mesh node references a missing asset", i});
    }
  }
  if (options.require_unique_names) {
    std::map<std::string, std::uint32_t> seen;
    for (std::uint32_t i = 0; i < scene.nodes.size(); ++i) {
      const std::string name(scene.string_at(scene.nodes[i].name).value_or(""));
      if (name.empty()) {
        continue;
      }
      if (seen.contains(name)) {
        findings.push_back({"unique-names", Severity::Warning, "duplicate node name: " + name, i});
      } else {
        seen[name] = i;
      }
    }
  }
  return findings;
}

std::string audit_to_text(const Scene& scene, std::span<const AuditFinding> findings) {
  std::ostringstream out;
  for (const AuditFinding& finding : findings) {
    out << severity_name(finding.severity) << " " << finding.rule;
    if (finding.node != kNoIndex && finding.node < scene.nodes.size()) {
      out << " node=" << scene.string_at(scene.nodes[finding.node].name).value_or("<unnamed>");
    }
    out << ": " << finding.message << "\n";
  }
  return out.str();
}

}  // namespace knotwork
