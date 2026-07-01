#pragma once

#include "knotwork/animation.hpp"

namespace knotwork {

struct ClipSummary {
  std::uint32_t track_count = 0;
  std::uint32_t key_count = 0;
  float first_time = 0.0f;
  float last_time = 0.0f;
};

[[nodiscard]] ClipSummary summarize_clip(const AnimationClip& clip);
[[nodiscard]] std::optional<std::uint32_t> find_track(const AnimationClip& clip, std::uint32_t node);
void sort_keys(NodeTrack& track);
bool insert_key(NodeTrack& track, TransformKey key, bool replace_existing = true);
bool remove_key(NodeTrack& track, float time, float epsilon = 0.0001f);
void shift_clip(AnimationClip& clip, float delta);
void scale_clip_time(AnimationClip& clip, float factor);
void clamp_clip(AnimationClip& clip, float start, float end);
[[nodiscard]] AnimationClip merge_clips(const AnimationClip& a, const AnimationClip& b, std::string name);

}  // namespace knotwork
