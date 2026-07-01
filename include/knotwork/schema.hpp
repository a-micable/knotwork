#pragma once

#include "knotwork/scene.hpp"

#include <span>
#include <string>
#include <vector>

namespace knotwork {

struct FieldSchema {
  std::string name;
  std::string type;
  std::string description;
  bool required = true;
};

struct TypeSchema {
  std::string name;
  std::string description;
  std::vector<FieldSchema> fields;
};

[[nodiscard]] std::vector<TypeSchema> scene_schema();
[[nodiscard]] const TypeSchema* find_type_schema(std::span<const TypeSchema> schema, std::string_view name);
[[nodiscard]] std::string schema_markdown(std::span<const TypeSchema> schema);
[[nodiscard]] std::string schema_text(std::span<const TypeSchema> schema);
[[nodiscard]] std::vector<std::string> schema_type_names(std::span<const TypeSchema> schema);

}  // namespace knotwork
