#pragma once

#include "knotwork/format.hpp"
#include "knotwork/scene.hpp"

namespace knotwork {

[[nodiscard]] SceneError validate_scene(const Scene& scene);

}  // namespace knotwork
