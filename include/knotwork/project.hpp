#pragma once

#include "knotwork/package.hpp"
#include "knotwork/scene_inspector.hpp"

#include <filesystem>

namespace knotwork {

struct ProjectFile {
  std::filesystem::path path;
  PackageEntryKind kind = PackageEntryKind::Binary;
  std::string label;
};

struct Project {
  std::string name;
  std::filesystem::path root;
  std::vector<ProjectFile> files;
  Scene scene;
  AssetLibrary assets;
};

[[nodiscard]] Project make_project(std::string name, std::filesystem::path root);
void add_project_file(Project& project,
                      std::filesystem::path path,
                      PackageEntryKind kind,
                      std::string label = {});
[[nodiscard]] std::string project_manifest_text(const Project& project);
[[nodiscard]] Project package_to_project(const Package& package, std::string name = "package");
[[nodiscard]] Package project_to_package(const Project& project);
[[nodiscard]] SceneInspection inspect_project(const Project& project);

}  // namespace knotwork
