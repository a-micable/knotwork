#include "knotwork/schema.hpp"
#include "knotwork/version.hpp"

#include <iostream>

int main(int argc, char** argv) {
  const auto schema = knotwork::scene_schema();
  if (argc == 2 && std::string(argv[1]) == "--markdown") {
    std::cout << knotwork::schema_markdown(schema);
    return 0;
  }
  if (argc == 2 && std::string(argv[1]) == "--types") {
    for (const std::string& name : knotwork::schema_type_names(schema)) {
      std::cout << name << "\n";
    }
    return 0;
  }
  std::cout << knotwork::version_string() << "\n";
  std::cout << knotwork::build_capabilities();
  std::cout << knotwork::schema_text(schema);
  return 0;
}
