#include "knotwork/builder.hpp"

#include <algorithm>

namespace knotwork {

std::uint32_t SceneBuilder::intern(std::string_view value) {
  const auto found = std::find(scene_.strings.begin(), scene_.strings.end(), value);
  if (found != scene_.strings.end()) {
    return static_cast<std::uint32_t>(std::distance(scene_.strings.begin(), found));
  }
  scene_.strings.emplace_back(value);
  return static_cast<std::uint32_t>(scene_.strings.size() - 1);
}

std::uint32_t SceneBuilder::add_material(std::string_view name,
                                         Vec3 base_color,
                                         float roughness,
                                         float metallic) {
  Material material;
  material.name = intern(name);
  material.base_color = base_color;
  material.roughness = roughness;
  material.metallic = metallic;
  scene_.materials.push_back(material);
  return static_cast<std::uint32_t>(scene_.materials.size() - 1);
}

std::uint32_t SceneBuilder::add_node(std::string_view name,
                                     NodeKind kind,
                                     std::uint32_t parent,
                                     Transform local) {
  Node node;
  node.id = next_id_++;
  node.name = intern(name);
  node.kind = kind;
  node.parent = parent;
  node.local = local;
  scene_.nodes.push_back(node);
  return static_cast<std::uint32_t>(scene_.nodes.size() - 1);
}

std::uint32_t SceneBuilder::add_mesh(std::string_view name,
                                     std::uint32_t parent,
                                     std::uint32_t material,
                                     Transform local) {
  const std::uint32_t index = add_node(name, NodeKind::Mesh, parent, local);
  scene_.nodes[index].material = material;
  return index;
}

std::uint32_t SceneBuilder::add_light(std::string_view name,
                                      std::uint32_t parent,
                                      LightKind light_kind,
                                      float intensity,
                                      Transform local) {
  const std::uint32_t index = add_node(name, NodeKind::Light, parent, local);
  scene_.nodes[index].light_kind = light_kind;
  scene_.nodes[index].light_intensity = intensity;
  return index;
}

std::uint32_t SceneBuilder::add_camera(std::string_view name,
                                       std::uint32_t parent,
                                       float fov_y,
                                       Transform local) {
  const std::uint32_t index = add_node(name, NodeKind::Camera, parent, local);
  scene_.nodes[index].camera_fov_y = fov_y;
  return index;
}

std::uint32_t SceneBuilder::add_instance(std::string_view name,
                                         std::uint32_t parent,
                                         std::uint32_t target,
                                         Transform local) {
  const std::uint32_t index = add_node(name, NodeKind::Instance, parent, local);
  scene_.nodes[index].instance_target = target;
  return index;
}

const Scene& SceneBuilder::scene() const {
  return scene_;
}

Scene SceneBuilder::take() {
  Scene result = std::move(scene_);
  scene_ = {};
  next_id_ = 1;
  return result;
}

Transform translated(float x, float y, float z) {
  Transform transform;
  transform.translation = {x, y, z};
  return transform;
}

Transform scaled(float x, float y, float z) {
  Transform transform;
  transform.scale = {x, y, z};
  return transform;
}

Transform trs(Vec3 translation, Quat rotation, Vec3 scale) {
  Transform transform;
  transform.translation = translation;
  transform.rotation = rotation;
  transform.scale = scale;
  return transform;
}

}  // namespace knotwork
