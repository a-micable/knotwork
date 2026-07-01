#pragma once

#include "knotwork/asset_library.hpp"
#include "knotwork/mesh_io.hpp"

namespace knotwork {

struct AssetManifestEntry {
  std::string name;
  std::string source_uri;
  std::uint64_t hash = 0;
  std::uint32_t vertices = 0;
  std::uint32_t triangles = 0;
  std::vector<std::string> tags;
};

struct AssetManifest {
  std::vector<AssetManifestEntry> meshes;
};

[[nodiscard]] AssetManifest build_asset_manifest(const AssetLibrary& library);
[[nodiscard]] std::string asset_manifest_text(const AssetManifest& manifest);
[[nodiscard]] std::string asset_manifest_json(const AssetManifest& manifest);
[[nodiscard]] std::vector<AssetManifestEntry> changed_assets(const AssetManifest& before,
                                                             const AssetManifest& after);

}  // namespace knotwork
