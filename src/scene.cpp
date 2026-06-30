#include "knotwork/scene.hpp"

#include <algorithm>

namespace knotwork {

Mat4 Transform::matrix() const {
  return Mat4::compose(translation, rotation, scale);
}

std::optional<std::string_view> Scene::string_at(std::uint32_t index) const {
  if (index >= strings.size()) {
    return std::nullopt;
  }
  return strings[index];
}

std::optional<std::uint32_t> Scene::node_index_by_id(std::uint32_t id) const {
  const auto found = std::find_if(nodes.begin(), nodes.end(), [id](const Node& node) {
    return node.id == id;
  });
  if (found == nodes.end()) {
    return std::nullopt;
  }
  return static_cast<std::uint32_t>(std::distance(nodes.begin(), found));
}

std::vector<std::uint32_t> Scene::children_of(std::uint32_t parent_index) const {
  std::vector<std::uint32_t> result;
  for (std::uint32_t i = 0; i < nodes.size(); ++i) {
    if (nodes[i].parent == parent_index) {
      result.push_back(i);
    }
  }
  return result;
}

const char* node_kind_name(NodeKind kind) {
  switch (kind) {
    case NodeKind::Empty:
      return "empty";
    case NodeKind::Mesh:
      return "mesh";
    case NodeKind::Light:
      return "light";
    case NodeKind::Camera:
      return "camera";
    case NodeKind::Instance:
      return "instance";
  }
  return "unknown";
}

const char* light_kind_name(LightKind kind) {
  switch (kind) {
    case LightKind::Point:
      return "point";
    case LightKind::Directional:
      return "directional";
    case LightKind::Spot:
      return "spot";
  }
  return "unknown";
}

}  // namespace knotwork
