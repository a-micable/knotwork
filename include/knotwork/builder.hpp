#pragma once

#include "knotwork/scene.hpp"

#include <string>
#include <string_view>

namespace knotwork {

class SceneBuilder {
 public:
  std::uint32_t intern(std::string_view value);
  std::uint32_t add_material(std::string_view name,
                             Vec3 base_color = {1.0f, 1.0f, 1.0f},
                             float roughness = 0.5f,
                             float metallic = 0.0f);
  std::uint32_t add_node(std::string_view name,
                         NodeKind kind,
                         std::uint32_t parent = kNoIndex,
                         Transform local = {});
  std::uint32_t add_mesh(std::string_view name,
                         std::uint32_t parent,
                         std::uint32_t material,
                         Transform local = {});
  std::uint32_t add_light(std::string_view name,
                          std::uint32_t parent,
                          LightKind light_kind,
                          float intensity,
                          Transform local = {});
  std::uint32_t add_camera(std::string_view name,
                           std::uint32_t parent,
                           float fov_y,
                           Transform local = {});
  std::uint32_t add_instance(std::string_view name,
                             std::uint32_t parent,
                             std::uint32_t target,
                             Transform local = {});

  [[nodiscard]] const Scene& scene() const;
  [[nodiscard]] Scene take();

 private:
  Scene scene_;
  std::uint32_t next_id_ = 1;
};

Transform translated(float x, float y, float z);
Transform scaled(float x, float y, float z);
Transform trs(Vec3 translation, Quat rotation, Vec3 scale);

}  // namespace knotwork
