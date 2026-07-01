#pragma once

#include "knotwork/mesh.hpp"

#include <filesystem>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace knotwork {

struct MeshHandle {
  std::uint32_t index = kNoIndex;
};

struct MeshRecord {
  MeshHandle handle{};
  MeshAsset mesh{};
  std::string source_uri;
  std::vector<std::string> tags;
};

struct AssetLibraryStats {
  std::uint32_t mesh_count = 0;
  std::uint32_t vertex_count = 0;
  std::uint32_t triangle_count = 0;
  std::uint32_t tagged_mesh_count = 0;
  std::optional<Aabb> bounds;
};

struct MeshSearch {
  std::string name_contains;
  std::string tag;
  std::optional<std::uint32_t> material;
  std::uint32_t min_triangles = 0;
  std::uint32_t max_triangles = 0;
};

class AssetLibrary {
 public:
  [[nodiscard]] MeshHandle add_mesh(MeshAsset mesh,
                                    std::string source_uri = {},
                                    std::vector<std::string> tags = {});
  [[nodiscard]] bool erase_mesh(MeshHandle handle);
  [[nodiscard]] bool rename_mesh(MeshHandle handle, std::string_view name);
  [[nodiscard]] bool add_tag(MeshHandle handle, std::string tag);
  [[nodiscard]] bool remove_tag(MeshHandle handle, std::string_view tag);
  [[nodiscard]] std::optional<MeshHandle> find_mesh(std::string_view name) const;
  [[nodiscard]] const MeshAsset* get(MeshHandle handle) const;
  [[nodiscard]] MeshAsset* get(MeshHandle handle);
  [[nodiscard]] const MeshRecord* record(MeshHandle handle) const;
  [[nodiscard]] MeshRecord* record(MeshHandle handle);
  [[nodiscard]] std::vector<MeshHandle> handles() const;
  [[nodiscard]] std::vector<MeshHandle> search(const MeshSearch& search) const;
  [[nodiscard]] std::set<std::string> all_tags() const;
  [[nodiscard]] AssetLibraryStats stats() const;
  void clear();

 private:
  std::vector<std::optional<MeshRecord>> meshes_;
  std::map<std::string, MeshHandle, std::less<>> name_index_;
  std::uint32_t next_slot_ = 0;
};

[[nodiscard]] bool valid_handle(MeshHandle handle);
[[nodiscard]] std::string summarize_mesh(const MeshAsset& mesh);
[[nodiscard]] std::string summarize_library(const AssetLibrary& library);
[[nodiscard]] MeshAsset* mutable_mesh_or_null(AssetLibrary& library, MeshHandle handle);
[[nodiscard]] const MeshAsset* mesh_or_null(const AssetLibrary& library, MeshHandle handle);

}  // namespace knotwork
