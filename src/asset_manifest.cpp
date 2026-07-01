#include "knotwork/asset_manifest.hpp"

#include <sstream>

namespace knotwork {

AssetManifest build_asset_manifest(const AssetLibrary& library) {
  AssetManifest manifest;
  for (MeshHandle handle : library.handles()) {
    const MeshRecord* record = library.record(handle);
    if (record == nullptr) {
      continue;
    }
    const MeshStats stats = mesh_stats(record->mesh);
    AssetManifestEntry entry;
    entry.name = record->mesh.name;
    entry.source_uri = record->source_uri;
    entry.hash = mesh_content_hash(record->mesh);
    entry.vertices = stats.vertex_count;
    entry.triangles = stats.triangle_count;
    entry.tags = record->tags;
    manifest.meshes.push_back(std::move(entry));
  }
  return manifest;
}

std::string asset_manifest_text(const AssetManifest& manifest) {
  std::ostringstream out;
  out << "meshes=" << manifest.meshes.size() << "\n";
  for (const AssetManifestEntry& entry : manifest.meshes) {
    out << entry.name << " hash=" << entry.hash << " vertices=" << entry.vertices
        << " triangles=" << entry.triangles;
    if (!entry.source_uri.empty()) {
      out << " source=" << entry.source_uri;
    }
    if (!entry.tags.empty()) {
      out << " tags=";
      for (std::size_t i = 0; i < entry.tags.size(); ++i) {
        if (i != 0) {
          out << ",";
        }
        out << entry.tags[i];
      }
    }
    out << "\n";
  }
  return out.str();
}

std::string asset_manifest_json(const AssetManifest& manifest) {
  std::ostringstream out;
  out << "{\n  \"meshes\": [\n";
  for (std::size_t i = 0; i < manifest.meshes.size(); ++i) {
    const AssetManifestEntry& entry = manifest.meshes[i];
    out << "    {\"name\":\"" << entry.name << "\",\"source\":\"" << entry.source_uri
        << "\",\"hash\":" << entry.hash << ",\"vertices\":" << entry.vertices
        << ",\"triangles\":" << entry.triangles << ",\"tags\":[";
    for (std::size_t t = 0; t < entry.tags.size(); ++t) {
      if (t != 0) {
        out << ",";
      }
      out << "\"" << entry.tags[t] << "\"";
    }
    out << "]}";
    if (i + 1 != manifest.meshes.size()) {
      out << ",";
    }
    out << "\n";
  }
  out << "  ]\n}\n";
  return out.str();
}

std::vector<AssetManifestEntry> changed_assets(const AssetManifest& before, const AssetManifest& after) {
  std::vector<AssetManifestEntry> changed;
  for (const AssetManifestEntry& next : after.meshes) {
    bool found = false;
    for (const AssetManifestEntry& prev : before.meshes) {
      if (prev.name == next.name) {
        found = true;
        if (prev.hash != next.hash || prev.vertices != next.vertices || prev.triangles != next.triangles) {
          changed.push_back(next);
        }
        break;
      }
    }
    if (!found) {
      changed.push_back(next);
    }
  }
  return changed;
}

}  // namespace knotwork
