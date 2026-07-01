#pragma once

#include "knotwork/patch.hpp"

#include <string>
#include <vector>

namespace knotwork {

enum class DiffKind {
  StringAdded,
  StringRemoved,
  StringChanged,
  MaterialAdded,
  MaterialRemoved,
  MaterialChanged,
  NodeAdded,
  NodeRemoved,
  NodeChanged,
};

struct DiffEntry {
  DiffKind kind = DiffKind::StringChanged;
  std::uint32_t index = kNoIndex;
  std::string path;
  std::string before;
  std::string after;
};

struct SceneDiff {
  std::vector<DiffEntry> entries;
};

[[nodiscard]] const char* diff_kind_name(DiffKind kind);
[[nodiscard]] SceneDiff diff_scenes(const Scene& before, const Scene& after);
[[nodiscard]] bool empty(const SceneDiff& diff);
[[nodiscard]] std::string diff_to_text(const SceneDiff& diff);
[[nodiscard]] ScenePatch patch_from_diff(const Scene& before, const Scene& after);

}  // namespace knotwork
