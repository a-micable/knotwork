#pragma once

#include "knotwork/asset_manifest.hpp"
#include "knotwork/project.hpp"
#include "knotwork/validation_report.hpp"

namespace knotwork {

struct ConsistencyOptions {
  bool require_manifest_sources = false;
  bool require_mesh_for_mesh_nodes = true;
  bool require_project_files = false;
};

struct ConsistencyIssue {
  Severity severity = Severity::Info;
  std::string category;
  std::string message;
};

struct ConsistencyReport {
  std::vector<ConsistencyIssue> issues;
  [[nodiscard]] bool ok() const;
};

[[nodiscard]] ConsistencyReport check_project_consistency(const Project& project,
                                                          ConsistencyOptions options = {});
[[nodiscard]] std::string consistency_report_text(const ConsistencyReport& report);
[[nodiscard]] std::vector<ConsistencyIssue> asset_consistency_issues(const Project& project,
                                                                     ConsistencyOptions options);
[[nodiscard]] std::vector<ConsistencyIssue> file_consistency_issues(const Project& project,
                                                                    ConsistencyOptions options);

}  // namespace knotwork
