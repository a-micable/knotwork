#pragma once

#include "knotwork/scene_audit.hpp"
#include "knotwork/statistics.hpp"

namespace knotwork {

struct QualityScore {
  float score = 100.0f;
  std::vector<std::string> reasons;
};

struct QualityWeights {
  float error_penalty = 25.0f;
  float warning_penalty = 8.0f;
  float missing_asset_penalty = 12.0f;
  float degenerate_penalty = 3.0f;
};

[[nodiscard]] QualityScore score_project_quality(const Project& project,
                                                 QualityWeights weights = {});
[[nodiscard]] std::string quality_to_text(const QualityScore& score);

}  // namespace knotwork
