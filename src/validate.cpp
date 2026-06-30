#include "knotwork/validate.hpp"

#include <cmath>
#include <unordered_set>
#include <vector>

namespace knotwork {
namespace {

SceneError ok() {
  return {};
}

SceneError error(SceneErrorCode code, std::string message) {
  return {code, std::move(message)};
}

bool finite(Vec3 value) {
  return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

bool finite(Quat value) {
  return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z) &&
         std::isfinite(value.w);
}

bool valid_string_ref(const Scene& scene, std::uint32_t index) {
  return index == kNoIndex || index < scene.strings.size();
}

bool valid_node_ref(const Scene& scene, std::uint32_t index) {
  return index == kNoIndex || index < scene.nodes.size();
}

bool valid_material_ref(const Scene& scene, std::uint32_t index) {
  return index == kNoIndex || index < scene.materials.size();
}

SceneError detect_parent_cycles(const Scene& scene) {
  enum class Mark : std::uint8_t { Unvisited, Visiting, Done };
  std::vector<Mark> marks(scene.nodes.size(), Mark::Unvisited);

  auto visit = [&](auto&& self, std::uint32_t index) -> SceneError {
    if (marks[index] == Mark::Visiting) {
      return error(SceneErrorCode::Cycle, "parent hierarchy contains a cycle");
    }
    if (marks[index] == Mark::Done) {
      return ok();
    }
    marks[index] = Mark::Visiting;
    const std::uint32_t parent = scene.nodes[index].parent;
    if (parent != kNoIndex) {
      const SceneError nested = self(self, parent);
      if (nested.code != SceneErrorCode::None) {
        return nested;
      }
    }
    marks[index] = Mark::Done;
    return ok();
  };

  for (std::uint32_t i = 0; i < scene.nodes.size(); ++i) {
    const SceneError result = visit(visit, i);
    if (result.code != SceneErrorCode::None) {
      return result;
    }
  }
  return ok();
}

SceneError detect_instance_cycles(const Scene& scene) {
  enum class Mark : std::uint8_t { Unvisited, Visiting, Done };
  std::vector<Mark> marks(scene.nodes.size(), Mark::Unvisited);

  auto visit = [&](auto&& self, std::uint32_t index) -> SceneError {
    if (marks[index] == Mark::Visiting) {
      return error(SceneErrorCode::Cycle, "instance references contain a cycle");
    }
    if (marks[index] == Mark::Done) {
      return ok();
    }
    marks[index] = Mark::Visiting;
    const Node& node = scene.nodes[index];
    if (node.kind == NodeKind::Instance && node.instance_target != kNoIndex) {
      const SceneError nested = self(self, node.instance_target);
      if (nested.code != SceneErrorCode::None) {
        return nested;
      }
    }
    marks[index] = Mark::Done;
    return ok();
  };

  for (std::uint32_t i = 0; i < scene.nodes.size(); ++i) {
    const SceneError result = visit(visit, i);
    if (result.code != SceneErrorCode::None) {
      return result;
    }
  }
  return ok();
}

}  // namespace

SceneError validate_scene(const Scene& scene) {
  std::unordered_set<std::uint32_t> ids;
  ids.reserve(scene.nodes.size());

  for (std::uint32_t i = 0; i < scene.materials.size(); ++i) {
    const Material& material = scene.materials[i];
    if (!valid_string_ref(scene, material.name)) {
      return error(SceneErrorCode::InvalidReference, "material name string index is out of range");
    }
    if (!finite(material.base_color) || !std::isfinite(material.roughness) ||
        !std::isfinite(material.metallic)) {
      return error(SceneErrorCode::InvalidReference, "material contains a non-finite value");
    }
  }

  for (std::uint32_t i = 0; i < scene.nodes.size(); ++i) {
    const Node& node = scene.nodes[i];
    if (!ids.insert(node.id).second) {
      return error(SceneErrorCode::DuplicateNodeId, "node IDs must be unique");
    }
    if (!valid_string_ref(scene, node.name) || !valid_node_ref(scene, node.parent) ||
        !valid_node_ref(scene, node.instance_target) || !valid_material_ref(scene, node.material)) {
      return error(SceneErrorCode::InvalidReference, "node contains an out-of-range table reference");
    }
    if (node.parent == i) {
      return error(SceneErrorCode::Cycle, "node cannot be its own parent");
    }
    if (node.kind != NodeKind::Instance && node.instance_target != kNoIndex) {
      return error(SceneErrorCode::InvalidReference, "only instance nodes may reference an instance target");
    }
    if (node.kind == NodeKind::Instance && node.instance_target == kNoIndex) {
      return error(SceneErrorCode::InvalidReference, "instance node is missing its target");
    }
    if (!finite(node.local.translation) || !finite(node.local.rotation) || !finite(node.local.scale) ||
        !std::isfinite(node.light_intensity) || !std::isfinite(node.camera_fov_y)) {
      return error(SceneErrorCode::InvalidReference, "node contains a non-finite transform or payload");
    }
  }

  SceneError parent = detect_parent_cycles(scene);
  if (parent.code != SceneErrorCode::None) {
    return parent;
  }
  return detect_instance_cycles(scene);
}

}  // namespace knotwork
