#pragma once

#include "knotwork/format.hpp"

#include <optional>
#include <vector>

namespace knotwork {

enum class Interpolation {
  Step,
  Linear,
  Smooth,
};

struct TransformKey {
  float time = 0.0f;
  Transform transform{};
  Interpolation interpolation = Interpolation::Linear;
};

struct NodeTrack {
  std::uint32_t node = kNoIndex;
  std::vector<TransformKey> keys;
};

struct AnimationClip {
  std::string name;
  float duration = 0.0f;
  std::vector<NodeTrack> tracks;
};

[[nodiscard]] Transform interpolate(const TransformKey& a, const TransformKey& b, float time);
[[nodiscard]] std::optional<Transform> sample_track(const NodeTrack& track, float time);
[[nodiscard]] Scene apply_clip(const Scene& scene, const AnimationClip& clip, float time);
[[nodiscard]] SceneError validate_clip(const Scene& scene, const AnimationClip& clip);

}  // namespace knotwork
