#include "knotwork/format.hpp"
#include "knotwork/scene_script.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

namespace {

int usage() {
  std::cerr << "usage: knotwork-script <input.kws> <output.kwscene>\n";
  std::cerr << "       knotwork-script --export <input.kwscene>\n";
  return 2;
}

std::string read_text(const std::filesystem::path& path) {
  std::ifstream input(path);
  std::ostringstream buffer;
  buffer << input.rdbuf();
  return buffer.str();
}

}  // namespace

int main(int argc, char** argv) {
  if (argc == 3 && std::string(argv[1]) == "--export") {
    auto scene = knotwork::load_scene_file(argv[2]);
    if (!scene.ok()) {
      std::cerr << scene.error().message << "\n";
      return 1;
    }
    std::cout << knotwork::scene_to_script(scene.value());
    return 0;
  }
  if (argc != 3) {
    return usage();
  }
  knotwork::ScriptResult result = knotwork::scene_from_script(read_text(argv[1]));
  if (knotwork::script_has_errors(result)) {
    for (const knotwork::ScriptIssue& issue : result.issues) {
      std::cerr << "line " << issue.line << ": " << issue.message << "\n";
    }
    return 1;
  }
  knotwork::SceneError error;
  if (!knotwork::save_scene_file(argv[2], result.scene, error)) {
    std::cerr << error.message << "\n";
    return 1;
  }
  return 0;
}
