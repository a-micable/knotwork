#include "knotwork/material.hpp"
#include "knotwork/mesh.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace knotwork {

std::uint32_t MaterialLibrary::add(MaterialPreset preset) {
  std::uint32_t index = kNoIndex;
  for (std::uint32_t i = 0; i < presets_.size(); ++i) {
    if (!presets_[i]) {
      index = i;
      break;
    }
  }
  if (index == kNoIndex) {
    index = static_cast<std::uint32_t>(presets_.size());
    presets_.push_back(std::nullopt);
  }
  names_.erase(preset.name);
  names_[preset.name] = index;
  presets_[index] = std::move(preset);
  return index;
}

bool MaterialLibrary::erase(std::uint32_t index) {
  if (index >= presets_.size() || !presets_[index]) {
    return false;
  }
  names_.erase(presets_[index]->name);
  presets_[index] = std::nullopt;
  return true;
}

const MaterialPreset* MaterialLibrary::get(std::uint32_t index) const {
  if (index >= presets_.size() || !presets_[index]) {
    return nullptr;
  }
  return &*presets_[index];
}

MaterialPreset* MaterialLibrary::get(std::uint32_t index) {
  if (index >= presets_.size() || !presets_[index]) {
    return nullptr;
  }
  return &*presets_[index];
}

std::optional<std::uint32_t> MaterialLibrary::find(std::string_view name) const {
  const auto found = names_.find(name);
  if (found == names_.end()) {
    return std::nullopt;
  }
  return found->second;
}

std::vector<std::uint32_t> MaterialLibrary::search_by_tag(std::string_view tag) const {
  std::vector<std::uint32_t> out;
  for (std::uint32_t i = 0; i < presets_.size(); ++i) {
    if (!presets_[i]) {
      continue;
    }
    if (std::find(presets_[i]->tags.begin(), presets_[i]->tags.end(), tag) != presets_[i]->tags.end()) {
      out.push_back(i);
    }
  }
  return out;
}

std::vector<MaterialPreset> MaterialLibrary::presets() const {
  std::vector<MaterialPreset> out;
  for (const auto& preset : presets_) {
    if (preset) {
      out.push_back(*preset);
    }
  }
  return out;
}

std::uint32_t MaterialLibrary::size() const {
  std::uint32_t count = 0;
  for (const auto& preset : presets_) {
    if (preset) {
      ++count;
    }
  }
  return count;
}

void MaterialLibrary::clear() {
  presets_.clear();
  names_.clear();
}

Material material_from_preset(SceneBuilder& builder, const MaterialPreset& preset) {
  Material material;
  material.name = builder.intern(preset.name);
  material.base_color = preset.base_color;
  material.roughness = preset.roughness;
  material.metallic = preset.metallic;
  return material;
}

std::vector<MaterialPreset> built_in_materials() {
  return {
      {"matte-white", {0.9f, 0.9f, 0.86f}, 0.95f, 0.0f, {"matte", "neutral"}},
      {"brushed-steel", {0.65f, 0.68f, 0.7f}, 0.35f, 1.0f, {"metal", "industrial"}},
      {"dark-rubber", {0.02f, 0.02f, 0.018f}, 0.8f, 0.0f, {"matte", "utility"}},
      {"warm-wood", {0.55f, 0.32f, 0.16f}, 0.6f, 0.0f, {"organic", "warm"}},
      {"signal-red", {1.0f, 0.04f, 0.02f}, 0.4f, 0.0f, {"paint", "warning"}},
  };
}

Vec3 srgb_to_linear(Vec3 color) {
  const auto convert = [](float value) {
    return value <= 0.04045f ? value / 12.92f : std::pow((value + 0.055f) / 1.055f, 2.4f);
  };
  return {convert(color.x), convert(color.y), convert(color.z)};
}

Vec3 linear_to_srgb(Vec3 color) {
  const auto convert = [](float value) {
    return value <= 0.0031308f ? value * 12.92f : 1.055f * std::pow(value, 1.0f / 2.4f) - 0.055f;
  };
  return {convert(color.x), convert(color.y), convert(color.z)};
}

Vec3 preview_lambert(Material material, Vec3 normal, Vec3 light_dir) {
  const float n_dot_l = std::max(0.0f, dot(normalize(normal), normalize(light_dir)));
  const float metal_tint = 0.35f + material.metallic * 0.65f;
  const float roughness_shadow = 1.0f - material.roughness * 0.25f;
  return material.base_color * (n_dot_l * metal_tint * roughness_shadow);
}

std::string material_palette_text(const MaterialLibrary& library) {
  std::ostringstream out;
  for (const MaterialPreset& preset : library.presets()) {
    out << preset.name << " color=" << preset.base_color.to_string() << " roughness=" << preset.roughness
        << " metallic=" << preset.metallic;
    if (!preset.tags.empty()) {
      out << " tags=";
      for (std::size_t i = 0; i < preset.tags.size(); ++i) {
        if (i != 0) {
          out << ",";
        }
        out << preset.tags[i];
      }
    }
    out << "\n";
  }
  return out.str();
}

}  // namespace knotwork
