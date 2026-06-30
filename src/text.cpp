#include "knotwork/text.hpp"
#include "knotwork/validate.hpp"

#include <charconv>
#include <sstream>
#include <string>
#include <unordered_map>

namespace knotwork {
namespace {

bool parse_u32(std::string_view text, std::uint32_t& out) {
  const auto* first = text.data();
  const auto* last = first + text.size();
  const auto result = std::from_chars(first, last, out);
  return result.ec == std::errc{} && result.ptr == last;
}

bool parse_float(std::string_view text, float& out) {
  const auto* first = text.data();
  const auto* last = first + text.size();
  const auto result = std::from_chars(first, last, out);
  return result.ec == std::errc{} && result.ptr == last;
}

std::vector<std::string_view> split(std::string_view line) {
  std::vector<std::string_view> out;
  while (!line.empty()) {
    const std::size_t next = line.find(' ');
    const std::string_view part = next == std::string_view::npos ? line : line.substr(0, next);
    if (!part.empty()) {
      out.push_back(part);
    }
    if (next == std::string_view::npos) {
      break;
    }
    line.remove_prefix(next + 1);
  }
  return out;
}

NodeKind parse_kind(std::string_view value) {
  if (value == "mesh") {
    return NodeKind::Mesh;
  }
  if (value == "light") {
    return NodeKind::Light;
  }
  if (value == "camera") {
    return NodeKind::Camera;
  }
  if (value == "instance") {
    return NodeKind::Instance;
  }
  return NodeKind::Empty;
}

SceneError error(std::string message) {
  return {SceneErrorCode::InvalidString, std::move(message)};
}

}  // namespace

std::string scene_to_text(const Scene& scene) {
  std::ostringstream out;
  out << "knotwork-text-v1\n";
  for (std::uint32_t i = 0; i < scene.strings.size(); ++i) {
    out << "string " << i << " " << scene.strings[i] << "\n";
  }
  for (std::uint32_t i = 0; i < scene.materials.size(); ++i) {
    const Material& material = scene.materials[i];
    out << "material " << i << " " << material.name << " " << material.base_color.x << " "
        << material.base_color.y << " " << material.base_color.z << " " << material.roughness
        << " " << material.metallic << "\n";
  }
  for (std::uint32_t i = 0; i < scene.nodes.size(); ++i) {
    const Node& node = scene.nodes[i];
    out << "node " << i << " " << node.id << " " << node.name << " "
        << node_kind_name(node.kind) << " " << node.parent << " " << node.material << " "
        << node.instance_target << " " << node.local.translation.x << " " << node.local.translation.y
        << " " << node.local.translation.z << " " << node.local.rotation.x << " "
        << node.local.rotation.y << " " << node.local.rotation.z << " " << node.local.rotation.w
        << " " << node.local.scale.x << " " << node.local.scale.y << " " << node.local.scale.z
        << "\n";
  }
  return out.str();
}

Result<Scene> scene_from_text(std::string_view text) {
  std::istringstream input{std::string(text)};
  std::string line;
  if (!std::getline(input, line) || line != "knotwork-text-v1") {
    return error("text scene is missing the knotwork-text-v1 header");
  }

  Scene scene;
  while (std::getline(input, line)) {
    if (line.empty()) {
      continue;
    }
    const auto parts = split(line);
    if (parts.empty()) {
      continue;
    }
    if (parts[0] == "string") {
      if (parts.size() < 3) {
        return error("string line has too few fields");
      }
      std::uint32_t index = 0;
      if (!parse_u32(parts[1], index) || index != scene.strings.size()) {
        return error("string indices must be contiguous");
      }
      scene.strings.emplace_back(parts[2]);
    } else if (parts[0] == "material") {
      if (parts.size() != 8) {
        return error("material line has the wrong number of fields");
      }
      Material material;
      std::uint32_t ignored = 0;
      if (!parse_u32(parts[1], ignored) || !parse_u32(parts[2], material.name) ||
          !parse_float(parts[3], material.base_color.x) ||
          !parse_float(parts[4], material.base_color.y) ||
          !parse_float(parts[5], material.base_color.z) ||
          !parse_float(parts[6], material.roughness) || !parse_float(parts[7], material.metallic)) {
        return error("material line contains invalid numeric data");
      }
      scene.materials.push_back(material);
    } else if (parts[0] == "node") {
      if (parts.size() != 18) {
        return error("node line has the wrong number of fields");
      }
      Node node;
      std::uint32_t ignored = 0;
      if (!parse_u32(parts[1], ignored) || !parse_u32(parts[2], node.id) ||
          !parse_u32(parts[3], node.name) || !parse_u32(parts[5], node.parent) ||
          !parse_u32(parts[6], node.material) || !parse_u32(parts[7], node.instance_target) ||
          !parse_float(parts[8], node.local.translation.x) ||
          !parse_float(parts[9], node.local.translation.y) ||
          !parse_float(parts[10], node.local.translation.z) ||
          !parse_float(parts[11], node.local.rotation.x) ||
          !parse_float(parts[12], node.local.rotation.y) ||
          !parse_float(parts[13], node.local.rotation.z) ||
          !parse_float(parts[14], node.local.rotation.w) ||
          !parse_float(parts[15], node.local.scale.x) ||
          !parse_float(parts[16], node.local.scale.y) ||
          !parse_float(parts[17], node.local.scale.z)) {
        return error("node line contains invalid numeric data");
      }
      node.kind = parse_kind(parts[4]);
      scene.nodes.push_back(node);
    } else {
      return error("unknown text scene directive");
    }
  }

  const SceneError validation = validate_scene(scene);
  if (validation.code != SceneErrorCode::None) {
    return validation;
  }
  return scene;
}

}  // namespace knotwork
