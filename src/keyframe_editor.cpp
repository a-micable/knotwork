#include "knotwork/keyframe_editor.hpp"

#include <algorithm>
#include <cmath>

namespace knotwork {

ClipSummary summarize_clip(const AnimationClip& clip) {
  ClipSummary summary;
  summary.track_count = static_cast<std::uint32_t>(clip.tracks.size());
  bool any = false;
  for (const NodeTrack& track : clip.tracks) {
    summary.key_count += static_cast<std::uint32_t>(track.keys.size());
    for (const TransformKey& key : track.keys) {
      if (!any) {
        summary.first_time = key.time;
        summary.last_time = key.time;
        any = true;
      } else {
        summary.first_time = std::min(summary.first_time, key.time);
        summary.last_time = std::max(summary.last_time, key.time);
      }
    }
  }
  return summary;
}

std::optional<std::uint32_t> find_track(const AnimationClip& clip, std::uint32_t node) {
  for (std::uint32_t i = 0; i < clip.tracks.size(); ++i) {
    if (clip.tracks[i].node == node) {
      return i;
    }
  }
  return std::nullopt;
}

void sort_keys(NodeTrack& track) {
  std::sort(track.keys.begin(), track.keys.end(), [](const TransformKey& lhs, const TransformKey& rhs) {
    return lhs.time < rhs.time;
  });
}

bool insert_key(NodeTrack& track, TransformKey key, bool replace_existing) {
  for (TransformKey& existing : track.keys) {
    if (std::fabs(existing.time - key.time) <= 0.0001f) {
      if (!replace_existing) {
        return false;
      }
      existing = key;
      sort_keys(track);
      return true;
    }
  }
  track.keys.push_back(key);
  sort_keys(track);
  return true;
}

bool remove_key(NodeTrack& track, float time, float epsilon) {
  const auto old_size = track.keys.size();
  track.keys.erase(std::remove_if(track.keys.begin(), track.keys.end(), [time, epsilon](const TransformKey& key) {
                     return std::fabs(key.time - time) <= epsilon;
                   }),
                   track.keys.end());
  return track.keys.size() != old_size;
}

void shift_clip(AnimationClip& clip, float delta) {
  for (NodeTrack& track : clip.tracks) {
    for (TransformKey& key : track.keys) {
      key.time += delta;
    }
  }
  clip.duration = std::max(0.0f, clip.duration + delta);
}

void scale_clip_time(AnimationClip& clip, float factor) {
  for (NodeTrack& track : clip.tracks) {
    for (TransformKey& key : track.keys) {
      key.time *= factor;
    }
  }
  clip.duration *= factor;
}

void clamp_clip(AnimationClip& clip, float start, float end) {
  for (NodeTrack& track : clip.tracks) {
    track.keys.erase(std::remove_if(track.keys.begin(), track.keys.end(), [start, end](const TransformKey& key) {
                       return key.time < start || key.time > end;
                     }),
                     track.keys.end());
    for (TransformKey& key : track.keys) {
      key.time -= start;
    }
  }
  clip.duration = std::max(0.0f, end - start);
}

AnimationClip merge_clips(const AnimationClip& a, const AnimationClip& b, std::string name) {
  AnimationClip out;
  out.name = std::move(name);
  out.duration = std::max(a.duration, b.duration);
  out.tracks = a.tracks;
  for (const NodeTrack& incoming : b.tracks) {
    auto index = find_track(out, incoming.node);
    if (!index) {
      out.tracks.push_back(incoming);
      continue;
    }
    for (const TransformKey& key : incoming.keys) {
      insert_key(out.tracks[*index], key, true);
    }
  }
  return out;
}

}  // namespace knotwork
