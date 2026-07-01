#include "knotwork/format.hpp"
#include "knotwork/json.hpp"
#include "knotwork/package.hpp"
#include "knotwork/text.hpp"
#include "knotwork/yaml.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>

namespace {

int usage() {
  std::cerr << "usage: knotwork-convert <input.kwscene> <json|text|yaml|package> [output]\n";
  return 2;
}

bool write_bytes(const std::filesystem::path& path, const std::vector<std::byte>& bytes) {
  std::ofstream output(path, std::ios::binary);
  if (!output) {
    return false;
  }
  output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  return static_cast<bool>(output);
}

bool write_text(const std::filesystem::path& path, const std::string& text) {
  std::ofstream output(path);
  if (!output) {
    return false;
  }
  output << text;
  return static_cast<bool>(output);
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 3 || argc > 4) {
    return usage();
  }
  auto loaded = knotwork::load_scene_file(std::filesystem::path(argv[1]));
  if (!loaded.ok()) {
    std::cerr << "load failed: " << loaded.error().message << "\n";
    return 1;
  }
  const std::string format = argv[2];
  const bool to_file = argc == 4;
  if (format == "json") {
    const std::string text = knotwork::scene_to_json(loaded.value());
    if (to_file) {
      return write_text(argv[3], text) ? 0 : 1;
    }
    std::cout << text;
    return 0;
  }
  if (format == "text") {
    const std::string text = knotwork::scene_to_text(loaded.value());
    if (to_file) {
      return write_text(argv[3], text) ? 0 : 1;
    }
    std::cout << text;
    return 0;
  }
  if (format == "yaml") {
    const std::string text = knotwork::scene_to_yaml(loaded.value());
    if (to_file) {
      return write_text(argv[3], text) ? 0 : 1;
    }
    std::cout << text;
    return 0;
  }
  if (format == "package") {
    knotwork::Package package;
    package.entries.push_back(knotwork::make_scene_entry("scene.kwscene", loaded.value()));
    std::vector<std::byte> bytes = knotwork::save_package(package);
    if (to_file) {
      return write_bytes(argv[3], bytes) ? 0 : 1;
    }
    std::cout.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    return 0;
  }
  return usage();
}
