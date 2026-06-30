#include "knotwork/flatten.hpp"
#include "knotwork/validate.hpp"

#include <vector>

namespace knotwork {
namespace {

enum class Visit : std::uint8_t { Unvisited, Visiting, Done };

SceneError error(SceneErrorCode code, std::string message) {
  return {code, std::move(message)};
}

Result<Mat4> resolve_node(const Scene& scene,
                          std::uint32_t index,
                          std::vector<Visit>& visiting,
                          std::vector<Mat4>& cache,
                          std::vector<bool>& has_cache,
                          const FlattenOptions& options) {
  if (index >= scene.nodes.size()) {
    return error(SceneErrorCode::InvalidReference, "node index is out of range");
  }
  if (has_cache[index]) {
    return cache[index];
  }
  if (visiting[index] == Visit::Visiting) {
    return error(SceneErrorCode::Cycle, "flattening encountered a graph cycle");
  }
  visiting[index] = Visit::Visiting;

  const Node& node = scene.nodes[index];
  Mat4 parent = Mat4::identity();
  if (node.parent != kNoIndex) {
    auto parent_result = resolve_node(scene, node.parent, visiting, cache, has_cache, options);
    if (!parent_result.ok()) {
      return parent_result.error();
    }
    parent = parent_result.value();
  }

  Mat4 local = node.local.matrix();
  if (options.include_instances && node.kind == NodeKind::Instance) {
    auto target_result = resolve_node(scene, node.instance_target, visiting, cache, has_cache, options);
    if (!target_result.ok()) {
      return target_result.error();
    }
    local = local * target_result.value();
  }

  cache[index] = parent * local;
  has_cache[index] = true;
  visiting[index] = Visit::Done;
  return cache[index];
}

}  // namespace

Result<std::vector<WorldNode>> flatten_scene(const Scene& scene, FlattenOptions options) {
  if (options.reject_cycles) {
    const SceneError validation = validate_scene(scene);
    if (validation.code != SceneErrorCode::None) {
      return validation;
    }
  }

  std::vector<WorldNode> out;
  out.reserve(scene.nodes.size());
  std::vector<Visit> visiting(scene.nodes.size(), Visit::Unvisited);
  std::vector<Mat4> cache(scene.nodes.size(), Mat4::identity());
  std::vector<bool> has_cache(scene.nodes.size(), false);

  for (std::uint32_t i = 0; i < scene.nodes.size(); ++i) {
    auto world = resolve_node(scene, i, visiting, cache, has_cache, options);
    if (!world.ok()) {
      return world.error();
    }
    out.push_back({scene.nodes[i].id, scene.nodes[i].kind, world.value()});
  }
  return out;
}

}  // namespace knotwork
