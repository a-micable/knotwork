#pragma once

#include "knotwork/selection.hpp"

namespace knotwork {

enum class LayoutAxis {
  X,
  Y,
  Z,
};

struct GridLayout {
  std::uint32_t columns = 4;
  float spacing_x = 2.0f;
  float spacing_z = 2.0f;
};

struct CircleLayout {
  float radius = 5.0f;
  LayoutAxis axis = LayoutAxis::Y;
};

[[nodiscard]] ScenePatch layout_grid_patch(const Scene& scene, const Selection& selection, GridLayout layout);
[[nodiscard]] ScenePatch layout_circle_patch(const Scene& scene, const Selection& selection, CircleLayout layout);
[[nodiscard]] ScenePatch align_patch(const Scene& scene,
                                     const Selection& selection,
                                     LayoutAxis axis,
                                     float coordinate);
[[nodiscard]] ScenePatch distribute_patch(const Scene& scene,
                                          const Selection& selection,
                                          LayoutAxis axis,
                                          float first,
                                          float last);
[[nodiscard]] Vec3 node_position(const Scene& scene, std::uint32_t node);
[[nodiscard]] Transform with_position(Transform transform, Vec3 position);

}  // namespace knotwork
