#pragma once

#include "knotwork/format.hpp"
#include "knotwork/scene.hpp"

#include <vector>

namespace knotwork {

struct FlattenOptions {
  bool include_instances = true;
  bool reject_cycles = true;
};

[[nodiscard]] Result<std::vector<WorldNode>> flatten_scene(const Scene& scene, FlattenOptions options = {});

}  // namespace knotwork
