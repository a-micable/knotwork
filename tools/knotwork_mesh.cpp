#include "knotwork/mesh_io.hpp"
#include "knotwork/obj.hpp"
#include "knotwork/procedural.hpp"
#include "knotwork/spatial_index.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>

namespace {

int usage() {
  std::cerr << "usage:\n";
  std::cerr << "  knotwork-mesh make <plane|box|cylinder|cone|grid> <output.kmesh>\n";
  std::cerr << "  knotwork-mesh info <input.kmesh>\n";
  std::cerr << "  knotwork-mesh obj <input.kmesh> <output.obj>\n";
  return 2;
}

bool write_text(const std::filesystem::path& path, const std::string& text) {
  std::ofstream output(path);
  if (!output) {
    return false;
  }
  output << text;
  return static_cast<bool>(output);
}

knotwork::MeshAsset make_shape(const std::string& shape) {
  if (shape == "plane") {
    return knotwork::make_plane_mesh("plane", 2.0f, 2.0f);
  }
  if (shape == "box") {
    return knotwork::make_box_mesh("box", {1.0f, 1.0f, 1.0f});
  }
  if (shape == "cylinder") {
    return knotwork::make_cylinder_mesh("cylinder", {});
  }
  if (shape == "cone") {
    return knotwork::make_cone_mesh("cone", {});
  }
  if (shape == "grid") {
    return knotwork::make_grid_mesh("grid", {});
  }
  return {};
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 3) {
    return usage();
  }
  const std::string command = argv[1];
  if (command == "make") {
    if (argc != 4) {
      return usage();
    }
    knotwork::MeshAsset mesh = make_shape(argv[2]);
    if (mesh.vertices.empty()) {
      std::cerr << "unknown shape\n";
      return 1;
    }
    knotwork::MeshIoError error;
    if (!knotwork::save_mesh_binary_file(argv[3], mesh, error)) {
      std::cerr << error.message << "\n";
      return 1;
    }
    return 0;
  }
  if (command == "info") {
    if (argc != 3) {
      return usage();
    }
    auto mesh = knotwork::load_mesh_binary_file(argv[2]);
    if (!mesh.ok()) {
      std::cerr << mesh.error().message << "\n";
      return 1;
    }
    const knotwork::MeshStats stats = knotwork::mesh_stats(mesh.value());
    knotwork::MeshBvh bvh = knotwork::build_mesh_bvh(mesh.value());
    std::cout << "name=" << mesh.value().name << "\n";
    std::cout << "vertices=" << stats.vertex_count << "\n";
    std::cout << "triangles=" << stats.triangle_count << "\n";
    std::cout << "area=" << stats.surface_area << "\n";
    std::cout << "bvh_depth=" << knotwork::bvh_depth(bvh) << "\n";
    std::cout << "bvh_leaves=" << knotwork::bvh_leaf_count(bvh) << "\n";
    return 0;
  }
  if (command == "obj") {
    if (argc != 4) {
      return usage();
    }
    auto mesh = knotwork::load_mesh_binary_file(argv[2]);
    if (!mesh.ok()) {
      std::cerr << mesh.error().message << "\n";
      return 1;
    }
    return write_text(argv[3], knotwork::save_obj(mesh.value())) ? 0 : 1;
  }
  return usage();
}
