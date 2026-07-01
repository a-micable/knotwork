#pragma once

#include "knotwork/asset_library.hpp"
#include "knotwork/format.hpp"

#include <cstddef>
#include <span>
#include <string>
#include <vector>

namespace knotwork {

enum class PackageEntryKind : std::uint8_t {
  Scene = 1,
  Mesh = 2,
  Text = 3,
  Binary = 4,
};

struct PackageEntry {
  PackageEntryKind kind = PackageEntryKind::Binary;
  std::string name;
  std::vector<std::byte> payload;
};

struct Package {
  std::vector<PackageEntry> entries;
};

[[nodiscard]] PackageEntry make_scene_entry(std::string name, const Scene& scene);
[[nodiscard]] PackageEntry make_mesh_obj_entry(std::string name, const MeshAsset& mesh);
[[nodiscard]] PackageEntry make_text_entry(std::string name, std::string_view text);
[[nodiscard]] std::vector<std::byte> save_package(const Package& package);
[[nodiscard]] Result<Package> load_package(std::span<const std::byte> bytes);
[[nodiscard]] std::optional<PackageEntry> find_entry(const Package& package, std::string_view name);
[[nodiscard]] std::string package_manifest(const Package& package);

}  // namespace knotwork
