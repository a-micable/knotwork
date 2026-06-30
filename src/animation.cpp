#include "knotwork/animation.hpp"

#include <algorithm>
#include <cmath>

namespace knotwork {
namespace {

float clamp01(float value) {
  return std::clamp(value, 0.0f, 1.0f);
}

float smooth(float value) {
  value = clamp01(value);
  return value * value * (3.0f - 2.0f * value);
}

float mix(float a, float b, float t) {
  return a + (b - a) * t;
}

Vec3 mix(Vec3 a, Vec3 b, float t) {
  return {mix(a.x, b.x, t), mix(a.y, b.y, t), mix(a.z, b.z, t)};
}

Quat mix(Quat a, Quat b, float t) {
  return {mix(a.x, b.x, t), mix(a.y, b.y, t), mix(a.z, b.z, t), mix(a.w, b.w, t)};
}

SceneError ok() {
  return {};
}

SceneError error(SceneErrorCode code, std::string message) {
  return {code, std::move(message)};
}

}  // namespace

Transform interpolate(const TransformKey& a, const TransformKey& b, float time) {
  if (b.time <= a.time || a.interpolation == Interpolation::Step) {
    return a.transform;
  }
  float t = (time - a.time) / (b.time - a.time);
  if (a.interpolation == Interpolation::Smooth) {
    t = smooth(t);
  } else {
    t = clamp01(t);
  }
  Transform out;
  out.translation = mix(a.transform.translation, b.transform.translation, t);
  out.rotation = mix(a.transform.rotation, b.transform.rotation, t).normalized();
  out.scale = mix(a.transform.scale, b.transform.scale, t);
  return out;
}

std::optional<Transform> sample_track(const NodeTrack& track, float time) {
  if (track.keys.empty()) {
    return std::nullopt;
  }
  if (time <= track.keys.front().time) {
    return track.keys.front().transform;
  }
  if (time >= track.keys.back().time) {
    return track.keys.back().transform;
  }
  for (std::size_t i = 1; i < track.keys.size(); ++i) {
    if (time <= track.keys[i].time) {
      return interpolate(track.keys[i - 1], track.keys[i], time);
    }
  }
  return track.keys.back().transform;
}

Scene apply_clip(const Scene& scene, const AnimationClip& clip, float time) {
  Scene out = scene;
  const float wrapped = clip.duration > 0.0f ? std::fmod(std::max(time, 0.0f), clip.duration) : time;
  for (const NodeTrack& track : clip.tracks) {
    if (track.node >= out.nodes.size()) {
      continue;
    }
    auto sampled = sample_track(track, wrapped);
    if (sampled) {
      out.nodes[track.node].local = *sampled;
    }
  }
  return out;
}

SceneError validate_clip(const Scene& scene, const AnimationClip& clip) {
  if (!std::isfinite(clip.duration) || clip.duration < 0.0f) {
    return error(SceneErrorCode::InvalidReference, "animation clip duration must be finite and non-negative");
  }
  for (const NodeTrack& track : clip.tracks) {
    if (track.node >= scene.nodes.size()) {
      return error(SceneErrorCode::InvalidReference, "animation track references an unknown node");
    }
    float previous = -std::numeric_limits<float>::infinity();
    for (const TransformKey& key : track.keys) {
      if (!std::isfinite(key.time) || key.time < previous) {
        return error(SceneErrorCode::InvalidReference, "animation keys must be sorted by finite time");
      }
      previous = key.time;
    }
  }
  return ok();
}

}  // namespace knotwork
