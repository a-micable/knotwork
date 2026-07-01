#include "knotwork/quality.hpp"

#include <algorithm>
#include <sstream>

namespace knotwork {

QualityScore score_project_quality(const Project& project, QualityWeights weights) {
  QualityScore score;
  const auto findings = audit_scene(project.scene, project.assets);
  for (const AuditFinding& finding : findings) {
    if (finding.severity == Severity::Error) {
      score.score -= weights.error_penalty;
    } else if (finding.severity == Severity::Warning) {
      score.score -= weights.warning_penalty;
    }
    score.reasons.push_back(finding.rule + ": " + finding.message);
  }
  const AssetStatistics assets = collect_asset_statistics(project.assets);
  if (assets.degenerate_triangle_count != 0) {
    score.score -= static_cast<float>(assets.degenerate_triangle_count) * weights.degenerate_penalty;
    score.reasons.push_back("degenerate triangles present");
  }
  for (const std::string& note : missing_asset_notes(project.scene, project.assets)) {
    score.score -= weights.missing_asset_penalty;
    score.reasons.push_back(note);
  }
  score.score = std::clamp(score.score, 0.0f, 100.0f);
  if (score.reasons.empty()) {
    score.reasons.push_back("no quality issues detected");
  }
  return score;
}

std::string quality_to_text(const QualityScore& score) {
  std::ostringstream out;
  out << "quality=" << score.score << "\n";
  for (const std::string& reason : score.reasons) {
    out << "- " << reason << "\n";
  }
  return out.str();
}

}  // namespace knotwork
