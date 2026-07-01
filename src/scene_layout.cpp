#include "knotwork/scene_layout.hpp"

#include <cmath>

namespace knotwork {
namespace {

void set_axis(Vec3& value, LayoutAxis axis, float coordinate) {
  if (axis == LayoutAxis::X) {
    value.x = coordinate;
  } else if (axis == LayoutAxis::Y) {
    value.y = coordinate;
  } else {
    value.z = coordinate;
  }
}

Vec3 circle_position(float angle, float radius, LayoutAxis axis) {
  const float a = std::cos(angle) * radius;
  const float b = std::sin(angle) * radius;
  if (axis == LayoutAxis::Y) {
    return {a, 0.0f, b};
  }
  if (axis == LayoutAxis::X) {
    return {0.0f, a, b};
  }
  return {a, b, 0.0f};
}

}  // namespace

Vec3 node_position(const Scene& scene, std::uint32_t node) {
  if (node >= scene.nodes.size()) {
    return {};
  }
  return scene.nodes[node].local.translation;
}

Transform with_position(Transform transform, Vec3 position) {
  transform.translation = position;
  return transform;
}

ScenePatch layout_grid_patch(const Scene& scene, const Selection& selection, GridLayout layout) {
  ScenePatch patch;
  patch.name = "layout-grid";
  const std::uint32_t columns = std::max<std::uint32_t>(layout.columns, 1);
  std::uint32_t i = 0;
  for (std::uint32_t node : selection.nodes) {
    if (node >= scene.nodes.size()) {
      continue;
    }
    const std::uint32_t column = i % columns;
    const std::uint32_t row = i / columns;
    PatchOp op;
    op.kind = PatchOpKind::SetTransform;
    op.index = node;
    op.transform = with_position(scene.nodes[node].local,
                                 {static_cast<float>(column) * layout.spacing_x,
                                  scene.nodes[node].local.translation.y,
                                  static_cast<float>(row) * layout.spacing_z});
    patch.ops.push_back(op);
    ++i;
  }
  return patch;
}

ScenePatch layout_circle_patch(const Scene& scene, const Selection& selection, CircleLayout layout) {
  ScenePatch patch;
  patch.name = "layout-circle";
  const float count = static_cast<float>(std::max<std::size_t>(selection.nodes.size(), 1));
  std::uint32_t i = 0;
  for (std::uint32_t node : selection.nodes) {
    if (node >= scene.nodes.size()) {
      continue;
    }
    const float angle = (static_cast<float>(i) / count) * 6.28318530718f;
    PatchOp op;
    op.kind = PatchOpKind::SetTransform;
    op.index = node;
    op.transform = with_position(scene.nodes[node].local, circle_position(angle, layout.radius, layout.axis));
    patch.ops.push_back(op);
    ++i;
  }
  return patch;
}

ScenePatch align_patch(const Scene& scene, const Selection& selection, LayoutAxis axis, float coordinate) {
  ScenePatch patch;
  patch.name = "align";
  for (std::uint32_t node : selection.nodes) {
    if (node >= scene.nodes.size()) {
      continue;
    }
    Vec3 position = scene.nodes[node].local.translation;
    set_axis(position, axis, coordinate);
    PatchOp op;
    op.kind = PatchOpKind::SetTransform;
    op.index = node;
    op.transform = with_position(scene.nodes[node].local, position);
    patch.ops.push_back(op);
  }
  return patch;
}

ScenePatch distribute_patch(const Scene& scene,
                            const Selection& selection,
                            LayoutAxis axis,
                            float first,
                            float last) {
  ScenePatch patch;
  patch.name = "distribute";
  const float denominator = selection.nodes.size() > 1 ? static_cast<float>(selection.nodes.size() - 1) : 1.0f;
  std::uint32_t i = 0;
  for (std::uint32_t node : selection.nodes) {
    if (node >= scene.nodes.size()) {
      continue;
    }
    const float t = static_cast<float>(i) / denominator;
    Vec3 position = scene.nodes[node].local.translation;
    set_axis(position, axis, first + (last - first) * t);
    PatchOp op;
    op.kind = PatchOpKind::SetTransform;
    op.index = node;
    op.transform = with_position(scene.nodes[node].local, position);
    patch.ops.push_back(op);
    ++i;
  }
  return patch;
}

}  // namespace knotwork
