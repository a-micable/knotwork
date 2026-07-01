#include "knotwork/validation_report.hpp"
#include "knotwork/graph.hpp"
#include "knotwork/validate.hpp"

#include <cmath>
#include <map>
#include <sstream>

namespace knotwork {
namespace {

ValidationFinding finding(Severity severity, SceneErrorCode code, std::string path, std::string message) {
  return {severity, code, std::move(path), std::move(message)};
}

bool finite(Vec3 value) {
  return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

bool finite(Quat value) {
  return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z) &&
         std::isfinite(value.w);
}

}  // namespace

bool ValidationReport::ok() const {
  for (const ValidationFinding& item : findings) {
    if (item.severity == Severity::Error) {
      return false;
    }
  }
  return true;
}

std::vector<ValidationFinding> reference_findings(const Scene& scene) {
  std::vector<ValidationFinding> out;
  for (std::uint32_t i = 0; i < scene.materials.size(); ++i) {
    if (scene.materials[i].name != kNoIndex && scene.materials[i].name >= scene.strings.size()) {
      out.push_back(finding(Severity::Error, SceneErrorCode::InvalidReference,
                            "materials[" + std::to_string(i) + "].name",
                            "material name index is outside the string table"));
    }
  }
  for (std::uint32_t i = 0; i < scene.nodes.size(); ++i) {
    const Node& node = scene.nodes[i];
    const std::string prefix = "nodes[" + std::to_string(i) + "]";
    if (node.name != kNoIndex && node.name >= scene.strings.size()) {
      out.push_back(finding(Severity::Error, SceneErrorCode::InvalidReference, prefix + ".name",
                            "node name index is outside the string table"));
    }
    if (node.parent != kNoIndex && node.parent >= scene.nodes.size()) {
      out.push_back(finding(Severity::Error, SceneErrorCode::InvalidReference, prefix + ".parent",
                            "parent index is outside the node table"));
    }
    if (node.material != kNoIndex && node.material >= scene.materials.size()) {
      out.push_back(finding(Severity::Warning, SceneErrorCode::InvalidReference, prefix + ".material",
                            "material index is outside the material table"));
    }
    if (node.instance_target != kNoIndex && node.instance_target >= scene.nodes.size()) {
      out.push_back(finding(Severity::Error, SceneErrorCode::InvalidReference, prefix + ".instance_target",
                            "instance target index is outside the node table"));
    }
    if (node.kind == NodeKind::Instance && node.instance_target == kNoIndex) {
      out.push_back(finding(Severity::Error, SceneErrorCode::InvalidReference, prefix + ".instance_target",
                            "instance node is missing a target"));
    }
  }
  return out;
}

std::vector<ValidationFinding> naming_findings(const Scene& scene) {
  std::vector<ValidationFinding> out;
  std::map<std::string, std::uint32_t> seen;
  for (std::uint32_t i = 0; i < scene.strings.size(); ++i) {
    if (scene.strings[i].empty()) {
      out.push_back(finding(Severity::Warning, SceneErrorCode::InvalidString,
                            "strings[" + std::to_string(i) + "]", "string entry is empty"));
    }
  }
  for (std::uint32_t i = 0; i < scene.nodes.size(); ++i) {
    const auto name = scene.string_at(scene.nodes[i].name);
    if (!name || name->empty()) {
      out.push_back(finding(Severity::Info, SceneErrorCode::None, "nodes[" + std::to_string(i) + "].name",
                            "node has no display name"));
      continue;
    }
    const std::string key(*name);
    if (seen.contains(key)) {
      out.push_back(finding(Severity::Warning, SceneErrorCode::None, "nodes[" + std::to_string(i) + "].name",
                            "node name duplicates nodes[" + std::to_string(seen[key]) + "]"));
    } else {
      seen[key] = i;
    }
  }
  return out;
}

std::vector<ValidationFinding> transform_findings(const Scene& scene) {
  std::vector<ValidationFinding> out;
  for (std::uint32_t i = 0; i < scene.nodes.size(); ++i) {
    const Transform& transform = scene.nodes[i].local;
    const std::string prefix = "nodes[" + std::to_string(i) + "].local";
    if (!finite(transform.translation)) {
      out.push_back(finding(Severity::Error, SceneErrorCode::InvalidReference, prefix + ".translation",
                            "translation contains non-finite values"));
    }
    if (!finite(transform.rotation)) {
      out.push_back(finding(Severity::Error, SceneErrorCode::InvalidReference, prefix + ".rotation",
                            "rotation contains non-finite values"));
    }
    if (!finite(transform.scale)) {
      out.push_back(finding(Severity::Error, SceneErrorCode::InvalidReference, prefix + ".scale",
                            "scale contains non-finite values"));
    }
    if (std::fabs(transform.scale.x) < 0.000001f || std::fabs(transform.scale.y) < 0.000001f ||
        std::fabs(transform.scale.z) < 0.000001f) {
      out.push_back(finding(Severity::Warning, SceneErrorCode::None, prefix + ".scale",
                            "scale is nearly zero on at least one axis"));
    }
  }
  return out;
}

ValidationReport validate_scene_detailed(const Scene& scene) {
  ValidationReport report;
  SceneError structural = validate_scene(scene);
  if (structural.code != SceneErrorCode::None) {
    report.findings.push_back(finding(Severity::Error, structural.code, "scene", structural.message));
  }
  auto refs = reference_findings(scene);
  report.findings.insert(report.findings.end(), refs.begin(), refs.end());
  auto names = naming_findings(scene);
  report.findings.insert(report.findings.end(), names.begin(), names.end());
  auto transforms = transform_findings(scene);
  report.findings.insert(report.findings.end(), transforms.begin(), transforms.end());
  if (root_nodes(scene).empty() && !scene.nodes.empty()) {
    report.findings.push_back(finding(Severity::Warning, SceneErrorCode::None, "nodes",
                                      "scene has nodes but no root"));
  }
  return report;
}

std::string validation_report_text(const ValidationReport& report) {
  std::ostringstream out;
  out << "validation=" << (report.ok() ? "ok" : "failed") << "\n";
  for (const ValidationFinding& item : report.findings) {
    out << severity_name(item.severity) << " " << scene_error_name(item.code) << " "
        << item.path << ": " << item.message << "\n";
  }
  return out.str();
}

}  // namespace knotwork
