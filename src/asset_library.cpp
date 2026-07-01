#include "knotwork/asset_library.hpp"

#include <algorithm>
#include <sstream>

namespace knotwork {
namespace {

bool contains_tag(const MeshRecord& record, std::string_view tag) {
  return std::find(record.tags.begin(), record.tags.end(), tag) != record.tags.end();
}

bool name_matches(const MeshRecord& record, std::string_view needle) {
  return needle.empty() || record.mesh.name.find(needle) != std::string::npos;
}

bool triangle_range_matches(const MeshRecord& record, std::uint32_t min_triangles, std::uint32_t max_triangles) {
  const std::uint32_t count = static_cast<std::uint32_t>(record.mesh.triangles.size());
  if (count < min_triangles) {
    return false;
  }
  if (max_triangles != 0 && count > max_triangles) {
    return false;
  }
  return true;
}

}  // namespace

MeshHandle AssetLibrary::add_mesh(MeshAsset mesh, std::string source_uri, std::vector<std::string> tags) {
  MeshHandle handle;
  for (std::uint32_t i = 0; i < meshes_.size(); ++i) {
    if (!meshes_[i]) {
      handle.index = i;
      break;
    }
  }
  if (!valid_handle(handle)) {
    handle.index = static_cast<std::uint32_t>(meshes_.size());
    meshes_.push_back(std::nullopt);
  }
  MeshRecord record;
  record.handle = handle;
  record.mesh = std::move(mesh);
  record.source_uri = std::move(source_uri);
  record.tags = std::move(tags);
  name_index_.erase(record.mesh.name);
  name_index_.emplace(record.mesh.name, handle);
  meshes_[handle.index] = std::move(record);
  next_slot_ = std::max(next_slot_, handle.index + 1);
  return handle;
}

bool AssetLibrary::erase_mesh(MeshHandle handle) {
  MeshRecord* item = record(handle);
  if (item == nullptr) {
    return false;
  }
  name_index_.erase(item->mesh.name);
  meshes_[handle.index] = std::nullopt;
  return true;
}

bool AssetLibrary::rename_mesh(MeshHandle handle, std::string_view name) {
  MeshRecord* item = record(handle);
  if (item == nullptr || name.empty()) {
    return false;
  }
  name_index_.erase(item->mesh.name);
  item->mesh.name = std::string(name);
  name_index_[item->mesh.name] = handle;
  return true;
}

bool AssetLibrary::add_tag(MeshHandle handle, std::string tag) {
  MeshRecord* item = record(handle);
  if (item == nullptr || tag.empty() || contains_tag(*item, tag)) {
    return false;
  }
  item->tags.push_back(std::move(tag));
  std::sort(item->tags.begin(), item->tags.end());
  return true;
}

bool AssetLibrary::remove_tag(MeshHandle handle, std::string_view tag) {
  MeshRecord* item = record(handle);
  if (item == nullptr) {
    return false;
  }
  const auto old_size = item->tags.size();
  item->tags.erase(std::remove(item->tags.begin(), item->tags.end(), tag), item->tags.end());
  return item->tags.size() != old_size;
}

std::optional<MeshHandle> AssetLibrary::find_mesh(std::string_view name) const {
  const auto found = name_index_.find(name);
  if (found == name_index_.end()) {
    return std::nullopt;
  }
  return found->second;
}

const MeshAsset* AssetLibrary::get(MeshHandle handle) const {
  const MeshRecord* item = record(handle);
  return item == nullptr ? nullptr : &item->mesh;
}

MeshAsset* AssetLibrary::get(MeshHandle handle) {
  MeshRecord* item = record(handle);
  return item == nullptr ? nullptr : &item->mesh;
}

const MeshRecord* AssetLibrary::record(MeshHandle handle) const {
  if (!valid_handle(handle) || handle.index >= meshes_.size() || !meshes_[handle.index]) {
    return nullptr;
  }
  return &*meshes_[handle.index];
}

MeshRecord* AssetLibrary::record(MeshHandle handle) {
  if (!valid_handle(handle) || handle.index >= meshes_.size() || !meshes_[handle.index]) {
    return nullptr;
  }
  return &*meshes_[handle.index];
}

std::vector<MeshHandle> AssetLibrary::handles() const {
  std::vector<MeshHandle> out;
  for (const auto& item : meshes_) {
    if (item) {
      out.push_back(item->handle);
    }
  }
  return out;
}

std::vector<MeshHandle> AssetLibrary::search(const MeshSearch& search) const {
  std::vector<MeshHandle> out;
  for (const auto& item : meshes_) {
    if (!item) {
      continue;
    }
    if (!name_matches(*item, search.name_contains)) {
      continue;
    }
    if (!search.tag.empty() && !contains_tag(*item, search.tag)) {
      continue;
    }
    if (search.material && item->mesh.material != *search.material) {
      continue;
    }
    if (!triangle_range_matches(*item, search.min_triangles, search.max_triangles)) {
      continue;
    }
    out.push_back(item->handle);
  }
  return out;
}

std::set<std::string> AssetLibrary::all_tags() const {
  std::set<std::string> tags;
  for (const auto& item : meshes_) {
    if (!item) {
      continue;
    }
    tags.insert(item->tags.begin(), item->tags.end());
  }
  return tags;
}

AssetLibraryStats AssetLibrary::stats() const {
  AssetLibraryStats stats;
  for (const auto& item : meshes_) {
    if (!item) {
      continue;
    }
    ++stats.mesh_count;
    if (!item->tags.empty()) {
      ++stats.tagged_mesh_count;
    }
    const MeshStats mesh = mesh_stats(item->mesh);
    stats.vertex_count += mesh.vertex_count;
    stats.triangle_count += mesh.triangle_count;
    if (mesh.bounds) {
      stats.bounds = stats.bounds ? merge(*stats.bounds, *mesh.bounds) : mesh.bounds;
    }
  }
  return stats;
}

void AssetLibrary::clear() {
  meshes_.clear();
  name_index_.clear();
  next_slot_ = 0;
}

bool valid_handle(MeshHandle handle) {
  return handle.index != kNoIndex;
}

std::string summarize_mesh(const MeshAsset& mesh) {
  const MeshStats stats = mesh_stats(mesh);
  std::ostringstream out;
  out << mesh.name << ": vertices=" << stats.vertex_count << " triangles=" << stats.triangle_count
      << " degenerates=" << stats.degenerate_count << " area=" << stats.surface_area;
  if (stats.bounds) {
    out << " bounds=(" << stats.bounds->min.to_string() << ")-(" << stats.bounds->max.to_string() << ")";
  }
  return out.str();
}

std::string summarize_library(const AssetLibrary& library) {
  const AssetLibraryStats stats = library.stats();
  std::ostringstream out;
  out << "meshes=" << stats.mesh_count << " vertices=" << stats.vertex_count
      << " triangles=" << stats.triangle_count << " tagged=" << stats.tagged_mesh_count;
  for (MeshHandle handle : library.handles()) {
    const MeshAsset* mesh = library.get(handle);
    if (mesh != nullptr) {
      out << "\n  " << summarize_mesh(*mesh);
    }
  }
  return out.str();
}

MeshAsset* mutable_mesh_or_null(AssetLibrary& library, MeshHandle handle) {
  return library.get(handle);
}

const MeshAsset* mesh_or_null(const AssetLibrary& library, MeshHandle handle) {
  return library.get(handle);
}

}  // namespace knotwork
