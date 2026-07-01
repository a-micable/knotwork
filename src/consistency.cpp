#include "knotwork/consistency.hpp"

#include <set>
#include <sstream>

namespace knotwork {
namespace {

ConsistencyIssue issue(Severity severity, std::string category, std::string message) {
  return {severity, std::move(category), std::move(message)};
}

}  // namespace

bool ConsistencyReport::ok() const {
  for (const ConsistencyIssue& item : issues) {
    if (item.severity == Severity::Error) {
      return false;
    }
  }
  return true;
}

std::vector<ConsistencyIssue> asset_consistency_issues(const Project& project, ConsistencyOptions options) {
  std::vector<ConsistencyIssue> issues;
  std::set<std::uint32_t> referenced;
  for (const Node& node : project.scene.nodes) {
    if ((node.kind == NodeKind::Mesh || node.kind == NodeKind::Instance) && node.material != kNoIndex) {
      referenced.insert(node.material);
    }
  }
  for (std::uint32_t asset : referenced) {
    if (project.assets.get(MeshHandle{asset}) == nullptr && options.require_mesh_for_mesh_nodes) {
      issues.push_back(issue(Severity::Warning, "assets",
                             "scene references mesh asset " + std::to_string(asset) + " that is not loaded"));
    }
  }
  AssetManifest manifest = build_asset_manifest(project.assets);
  for (const AssetManifestEntry& entry : manifest.meshes) {
    if (options.require_manifest_sources && entry.source_uri.empty()) {
      issues.push_back(issue(Severity::Warning, "assets", "asset " + entry.name + " has no source URI"));
    }
    if (entry.vertices == 0) {
      issues.push_back(issue(Severity::Error, "assets", "asset " + entry.name + " has no vertices"));
    }
    if (entry.triangles == 0) {
      issues.push_back(issue(Severity::Info, "assets", "asset " + entry.name + " has no triangles"));
    }
  }
  return issues;
}

std::vector<ConsistencyIssue> file_consistency_issues(const Project& project, ConsistencyOptions options) {
  std::vector<ConsistencyIssue> issues;
  if (options.require_project_files && project.files.empty()) {
    issues.push_back(issue(Severity::Warning, "files", "project has no tracked files"));
  }
  std::set<std::string> paths;
  for (const ProjectFile& file : project.files) {
    if (file.path.empty()) {
      issues.push_back(issue(Severity::Warning, "files", "project file has an empty path"));
      continue;
    }
    const std::string path = file.path.lexically_normal().string();
    if (!paths.insert(path).second) {
      issues.push_back(issue(Severity::Warning, "files", "duplicate project file path: " + path));
    }
  }
  return issues;
}

ConsistencyReport check_project_consistency(const Project& project, ConsistencyOptions options) {
  ConsistencyReport report;
  ValidationReport validation = validate_scene_detailed(project.scene);
  for (const ValidationFinding& finding : validation.findings) {
    if (finding.severity == Severity::Error) {
      report.issues.push_back(issue(Severity::Error, "scene", finding.path + ": " + finding.message));
    }
  }
  auto asset_issues = asset_consistency_issues(project, options);
  report.issues.insert(report.issues.end(), asset_issues.begin(), asset_issues.end());
  auto file_issues = file_consistency_issues(project, options);
  report.issues.insert(report.issues.end(), file_issues.begin(), file_issues.end());
  return report;
}

std::string consistency_report_text(const ConsistencyReport& report) {
  std::ostringstream out;
  out << "consistency=" << (report.ok() ? "ok" : "failed") << "\n";
  for (const ConsistencyIssue& item : report.issues) {
    out << severity_name(item.severity) << " " << item.category << ": " << item.message << "\n";
  }
  return out.str();
}

}  // namespace knotwork
