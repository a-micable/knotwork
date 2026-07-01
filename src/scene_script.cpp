#include "knotwork/scene_script.hpp"

#include <charconv>
#include <sstream>
#include <unordered_map>

namespace knotwork {
namespace {

std::vector<std::string_view> split_words(std::string_view line) {
  std::vector<std::string_view> out;
  while (!line.empty()) {
    while (!line.empty() && (line.front() == ' ' || line.front() == '\t' || line.front() == '\r')) {
      line.remove_prefix(1);
    }
    if (line.empty()) {
      break;
    }
    const std::size_t next = line.find_first_of(" \t\r");
    out.push_back(next == std::string_view::npos ? line : line.substr(0, next));
    if (next == std::string_view::npos) {
      break;
    }
    line.remove_prefix(next + 1);
  }
  return out;
}

bool parse_float(std::string_view text, float& out) {
  const auto result = std::from_chars(text.data(), text.data() + text.size(), out);
  return result.ec == std::errc{} && result.ptr == text.data() + text.size();
}

void issue(ScriptResult& result, std::uint32_t line, std::string message) {
  result.issues.push_back({line, std::move(message)});
}

NodeKind script_kind(std::string_view value) {
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

}  // namespace

ScriptResult scene_from_script(std::string_view script) {
  ScriptResult result;
  SceneBuilder builder;
  std::unordered_map<std::string, std::uint32_t> nodes;
  std::unordered_map<std::string, std::uint32_t> materials;
  std::istringstream input{std::string(script)};
  std::string line;
  std::uint32_t line_no = 0;
  while (std::getline(input, line)) {
    ++line_no;
    const auto words = split_words(line);
    if (words.empty() || words[0].starts_with("#")) {
      continue;
    }
    if (words[0] == "material") {
      if (words.size() < 5) {
        issue(result, line_no, "material requires name and rgb values");
        continue;
      }
      Vec3 color;
      if (!parse_float(words[2], color.x) || !parse_float(words[3], color.y) ||
          !parse_float(words[4], color.z)) {
        issue(result, line_no, "material rgb values are invalid");
        continue;
      }
      materials[std::string(words[1])] = builder.add_material(words[1], color);
    } else if (words[0] == "node") {
      if (words.size() < 7) {
        issue(result, line_no, "node requires name kind parent x y z");
        continue;
      }
      float x = 0.0f;
      float y = 0.0f;
      float z = 0.0f;
      if (!parse_float(words[4], x) || !parse_float(words[5], y) || !parse_float(words[6], z)) {
        issue(result, line_no, "node translation is invalid");
        continue;
      }
      std::uint32_t parent = kNoIndex;
      if (words[3] != "-") {
        const auto found = nodes.find(std::string(words[3]));
        if (found == nodes.end()) {
          issue(result, line_no, "node parent is unknown");
          continue;
        }
        parent = found->second;
      }
      const std::uint32_t node = builder.add_node(words[1], script_kind(words[2]), parent, translated(x, y, z));
      nodes[std::string(words[1])] = node;
    } else if (words[0] == "assign") {
      if (words.size() < 3) {
        issue(result, line_no, "assign requires node and material");
        continue;
      }
      const auto node = nodes.find(std::string(words[1]));
      const auto material = materials.find(std::string(words[2]));
      if (node == nodes.end() || material == materials.end()) {
        issue(result, line_no, "assign references unknown node or material");
        continue;
      }
      Scene temp = builder.take();
      temp.nodes[node->second].material = material->second;
      result.scene = temp;
    } else {
      issue(result, line_no, "unknown script directive");
    }
  }
  if (result.scene.nodes.empty()) {
    result.scene = builder.take();
  }
  return result;
}

std::string scene_to_script(const Scene& scene) {
  std::ostringstream out;
  for (const Material& material : scene.materials) {
    out << "material " << scene.string_at(material.name).value_or("material") << " "
        << material.base_color.x << " " << material.base_color.y << " " << material.base_color.z << "\n";
  }
  for (std::uint32_t i = 0; i < scene.nodes.size(); ++i) {
    const Node& node = scene.nodes[i];
    out << "node " << scene.string_at(node.name).value_or("node") << " " << node_kind_name(node.kind) << " ";
    if (node.parent == kNoIndex) {
      out << "-";
    } else {
      out << scene.string_at(scene.nodes[node.parent].name).value_or("-");
    }
    out << " " << node.local.translation.x << " " << node.local.translation.y << " "
        << node.local.translation.z << "\n";
  }
  return out.str();
}

bool script_has_errors(const ScriptResult& result) {
  return !result.issues.empty();
}

}  // namespace knotwork
