#include "knotwork/schema.hpp"

#include <sstream>

namespace knotwork {

std::vector<TypeSchema> scene_schema() {
  return {
      {"Vec3", "Three component floating-point vector.", {{"x", "float", "First component.", true}, {"y", "float", "Second component.", true}, {"z", "float", "Third component.", true}}},
      {"Quat", "Quaternion rotation.", {{"x", "float", "Imaginary x.", true}, {"y", "float", "Imaginary y.", true}, {"z", "float", "Imaginary z.", true}, {"w", "float", "Real component.", true}}},
      {"Transform", "Local transform.", {{"translation", "Vec3", "Local translation.", true}, {"rotation", "Quat", "Local orientation.", true}, {"scale", "Vec3", "Local scale.", true}}},
      {"Material", "Material payload.", {{"name", "uint32", "String table index.", false}, {"base_color", "Vec3", "Linear base color.", true}, {"roughness", "float", "Roughness factor.", true}, {"metallic", "float", "Metallic factor.", true}}},
      {"Node", "Scene graph node.", {{"id", "uint32", "Stable unique ID.", true}, {"name", "uint32", "String table index.", false}, {"kind", "NodeKind", "Node type.", true}, {"parent", "uint32", "Parent node index.", false}, {"material", "uint32", "Material or asset index.", false}, {"instance_target", "uint32", "Instance target node.", false}, {"local", "Transform", "Local transform.", true}, {"light_kind", "LightKind", "Light type.", false}, {"light_intensity", "float", "Light energy.", false}, {"camera_fov_y", "float", "Camera vertical FOV.", false}}},
      {"Scene", "Complete scene document.", {{"strings", "string[]", "Shared string table.", true}, {"materials", "Material[]", "Material table.", true}, {"nodes", "Node[]", "Node table.", true}}},
      {"MeshAsset", "Triangle mesh asset.", {{"name", "string", "Mesh name.", true}, {"vertices", "Vertex[]", "Vertex buffer.", true}, {"triangles", "Triangle[]", "Triangle buffer.", true}, {"material", "uint32", "Default material.", false}}},
      {"Package", "Archive of named entries.", {{"entries", "PackageEntry[]", "Scene, mesh, text, and binary payloads.", true}}},
  };
}

const TypeSchema* find_type_schema(std::span<const TypeSchema> schema, std::string_view name) {
  for (const TypeSchema& type : schema) {
    if (type.name == name) {
      return &type;
    }
  }
  return nullptr;
}

std::string schema_markdown(std::span<const TypeSchema> schema) {
  std::ostringstream out;
  out << "# Knotwork Schema\n\n";
  for (const TypeSchema& type : schema) {
    out << "## " << type.name << "\n\n" << type.description << "\n\n";
    out << "| Field | Type | Required | Description |\n| --- | --- | --- | --- |\n";
    for (const FieldSchema& field : type.fields) {
      out << "| `" << field.name << "` | `" << field.type << "` | "
          << (field.required ? "yes" : "no") << " | " << field.description << " |\n";
    }
    out << "\n";
  }
  return out.str();
}

std::string schema_text(std::span<const TypeSchema> schema) {
  std::ostringstream out;
  for (const TypeSchema& type : schema) {
    out << type.name << ": " << type.description << "\n";
    for (const FieldSchema& field : type.fields) {
      out << "  " << field.name << " (" << field.type << ")";
      if (!field.required) {
        out << " optional";
      }
      out << " - " << field.description << "\n";
    }
  }
  return out.str();
}

std::vector<std::string> schema_type_names(std::span<const TypeSchema> schema) {
  std::vector<std::string> names;
  for (const TypeSchema& type : schema) {
    names.push_back(type.name);
  }
  return names;
}

}  // namespace knotwork
