#include "knotwork/diagnostics.hpp"
#include "knotwork/graph.hpp"
#include "knotwork/validate.hpp"

#include <sstream>

namespace knotwork {

const char* severity_name(Severity severity) {
  switch (severity) {
    case Severity::Info:
      return "info";
    case Severity::Warning:
      return "warning";
    case Severity::Error:
      return "error";
  }
  return "unknown";
}

std::vector<Diagnostic> diagnose_scene(const Scene& scene) {
  std::vector<Diagnostic> out;
  const SceneError validation = validate_scene(scene);
  if (validation.code != SceneErrorCode::None) {
    out.push_back({Severity::Error, validation.code, validation.message, kNoIndex});
  }
  const GraphStats stats = graph_stats(scene);
  if (scene.nodes.empty()) {
    out.push_back({Severity::Warning, SceneErrorCode::None, "scene has no nodes", kNoIndex});
  }
  if (stats.roots == 0 && !scene.nodes.empty()) {
    out.push_back({Severity::Warning, SceneErrorCode::None, "scene has no root nodes", kNoIndex});
  }
  if (stats.camera_count == 0) {
    out.push_back({Severity::Info, SceneErrorCode::None, "scene has no camera nodes", kNoIndex});
  }
  if (stats.light_count == 0) {
    out.push_back({Severity::Info, SceneErrorCode::None, "scene has no light nodes", kNoIndex});
  }
  for (std::uint32_t i = 0; i < scene.nodes.size(); ++i) {
    const Node& node = scene.nodes[i];
    if (node.kind == NodeKind::Mesh && node.material == kNoIndex) {
      out.push_back({Severity::Warning, SceneErrorCode::InvalidReference, "mesh has no material", i});
    }
    if (node.kind == NodeKind::Light && node.light_intensity <= 0.0f) {
      out.push_back({Severity::Warning, SceneErrorCode::InvalidReference, "light intensity is not positive", i});
    }
    if (node.kind == NodeKind::Camera && node.camera_fov_y <= 0.0f) {
      out.push_back({Severity::Warning, SceneErrorCode::InvalidReference, "camera FOV is not positive", i});
    }
  }
  return out;
}

std::string format_diagnostic(const Scene& scene, const Diagnostic& diagnostic) {
  std::ostringstream out;
  out << severity_name(diagnostic.severity);
  if (diagnostic.code != SceneErrorCode::None) {
    out << "[" << scene_error_name(diagnostic.code) << "]";
  }
  if (diagnostic.node != kNoIndex && diagnostic.node < scene.nodes.size()) {
    out << " node=" << scene.string_at(scene.nodes[diagnostic.node].name).value_or("<unnamed>");
  }
  out << ": " << diagnostic.message;
  return out.str();
}

std::string format_diagnostics(const Scene& scene, std::span<const Diagnostic> diagnostics) {
  std::ostringstream out;
  for (const Diagnostic& diagnostic : diagnostics) {
    out << format_diagnostic(scene, diagnostic) << "\n";
  }
  return out.str();
}

}  // namespace knotwork
