#pragma once

#include "knotwork/format.hpp"

namespace knotwork {

[[nodiscard]] std::string scene_to_yaml(const Scene& scene);
[[nodiscard]] Result<Scene> scene_from_yaml(std::string_view yaml);

}  // namespace knotwork
