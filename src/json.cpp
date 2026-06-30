#include "knotwork/json.hpp"

#include <iomanip>
#include <sstream>

namespace knotwork {
namespace {

std::string escape_json(std::string_view value) {
  std::ostringstream out;
  for (char ch : value) {
    switch (ch) {
      case '"':
        out << "\\\"";
        break;
      case '\\':
        out << "\\\\";
        break;
      case '\n':
        out << "\\n";
        break;
      case '\r':
        out << "\\r";
        break;
      case '\t':
        out << "\\t";
        break;
      default:
        out << ch;
        break;
    }
  }
  return out.str();
}

void write_vec3(std::ostringstream& out, Vec3 value) {
  out << "[" << value.x << "," << value.y << "," << value.z << "]";
}

void write_quat(std::ostringstream& out, Quat value) {
  out << "[" << value.x << "," << value.y << "," << value.z << "," << value.w << "]";
}

}  // namespace

std::string scene_to_json(const Scene& scene) {
  std::ostringstream out;
  out << std::setprecision(6);
  out << "{\n  \"strings\": [";
  for (std::size_t i = 0; i < scene.strings.size(); ++i) {
    if (i != 0) {
      out << ", ";
    }
    out << "\"" << escape_json(scene.strings[i]) << "\"";
  }
  out << "],\n  \"materials\": [\n";
  for (std::size_t i = 0; i < scene.materials.size(); ++i) {
    const Material& material = scene.materials[i];
    out << "    {\"name\":" << material.name << ",\"baseColor\":";
    write_vec3(out, material.base_color);
    out << ",\"roughness\":" << material.roughness << ",\"metallic\":" << material.metallic << "}";
    if (i + 1 != scene.materials.size()) {
      out << ",";
    }
    out << "\n";
  }
  out << "  ],\n  \"nodes\": [\n";
  for (std::size_t i = 0; i < scene.nodes.size(); ++i) {
    const Node& node = scene.nodes[i];
    out << "    {\"id\":" << node.id << ",\"name\":" << node.name << ",\"kind\":\""
        << node_kind_name(node.kind) << "\",\"parent\":" << node.parent << ",\"material\":"
        << node.material << ",\"instanceTarget\":" << node.instance_target << ",\"translation\":";
    write_vec3(out, node.local.translation);
    out << ",\"rotation\":";
    write_quat(out, node.local.rotation);
    out << ",\"scale\":";
    write_vec3(out, node.local.scale);
    out << ",\"lightKind\":\"" << light_kind_name(node.light_kind) << "\",\"lightIntensity\":"
        << node.light_intensity << ",\"cameraFovY\":" << node.camera_fov_y << "}";
    if (i + 1 != scene.nodes.size()) {
      out << ",";
    }
    out << "\n";
  }
  out << "  ]\n}\n";
  return out.str();
}

}  // namespace knotwork
