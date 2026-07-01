#pragma once

#include "knotwork/builder.hpp"

#include <map>

namespace knotwork {

struct MaterialPreset {
  std::string name;
  Vec3 base_color{1.0f, 1.0f, 1.0f};
  float roughness = 0.5f;
  float metallic = 0.0f;
  std::vector<std::string> tags;
};

class MaterialLibrary {
 public:
  [[nodiscard]] std::uint32_t add(MaterialPreset preset);
  [[nodiscard]] bool erase(std::uint32_t index);
  [[nodiscard]] const MaterialPreset* get(std::uint32_t index) const;
  [[nodiscard]] MaterialPreset* get(std::uint32_t index);
  [[nodiscard]] std::optional<std::uint32_t> find(std::string_view name) const;
  [[nodiscard]] std::vector<std::uint32_t> search_by_tag(std::string_view tag) const;
  [[nodiscard]] std::vector<MaterialPreset> presets() const;
  [[nodiscard]] std::uint32_t size() const;
  void clear();

 private:
  std::vector<std::optional<MaterialPreset>> presets_;
  std::map<std::string, std::uint32_t, std::less<>> names_;
};

[[nodiscard]] Material material_from_preset(SceneBuilder& builder, const MaterialPreset& preset);
[[nodiscard]] std::vector<MaterialPreset> built_in_materials();
[[nodiscard]] Vec3 srgb_to_linear(Vec3 color);
[[nodiscard]] Vec3 linear_to_srgb(Vec3 color);
[[nodiscard]] Vec3 preview_lambert(Material material, Vec3 normal, Vec3 light_dir);
[[nodiscard]] std::string material_palette_text(const MaterialLibrary& library);

}  // namespace knotwork
