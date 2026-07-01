#include "knotwork/yaml.hpp"
#include "knotwork/text.hpp"

#include <sstream>

namespace knotwork {

std::string scene_to_yaml(const Scene& scene) {
  std::ostringstream out;
  out << "knotwork: 1\n";
  out << "strings:\n";
  for (std::uint32_t i = 0; i < scene.strings.size(); ++i) {
    out << "  - id: " << i << "\n";
    out << "    value: " << scene.strings[i] << "\n";
  }
  out << "materials:\n";
  for (std::uint32_t i = 0; i < scene.materials.size(); ++i) {
    const Material& material = scene.materials[i];
    out << "  - id: " << i << "\n";
    out << "    name: " << material.name << "\n";
    out << "    color: [" << material.base_color.x << ", " << material.base_color.y << ", "
        << material.base_color.z << "]\n";
    out << "    roughness: " << material.roughness << "\n";
    out << "    metallic: " << material.metallic << "\n";
  }
  out << "nodes:\n";
  for (std::uint32_t i = 0; i < scene.nodes.size(); ++i) {
    const Node& node = scene.nodes[i];
    out << "  - id: " << node.id << "\n";
    out << "    name: " << node.name << "\n";
    out << "    kind: " << node_kind_name(node.kind) << "\n";
    out << "    parent: " << node.parent << "\n";
    out << "    material: " << node.material << "\n";
    out << "    instance: " << node.instance_target << "\n";
    out << "    translation: [" << node.local.translation.x << ", " << node.local.translation.y
        << ", " << node.local.translation.z << "]\n";
  }
  return out.str();
}

Result<Scene> scene_from_yaml(std::string_view yaml) {
  // The YAML writer is intended for readable manifests. For now the importer
  // accepts the stable text-scene payload when embedded after a document marker.
  const std::string marker = "--- knotwork-text\n";
  const std::size_t found = yaml.find(marker);
  if (found == std::string_view::npos) {
    return SceneError{SceneErrorCode::InvalidString, "yaml import requires an embedded text scene"};
  }
  yaml.remove_prefix(found + marker.size());
  return scene_from_text(yaml);
}

}  // namespace knotwork
