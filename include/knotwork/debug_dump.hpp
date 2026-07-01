#pragma once

#include "knotwork/project.hpp"

namespace knotwork {

struct DumpOptions {
  bool include_strings = true;
  bool include_materials = true;
  bool include_nodes = true;
  bool include_assets = true;
  bool include_matrices = false;
};

[[nodiscard]] std::string dump_scene(const Scene& scene, DumpOptions options = {});
[[nodiscard]] std::string dump_assets(const AssetLibrary& assets);
[[nodiscard]] std::string dump_project(const Project& project, DumpOptions options = {});
[[nodiscard]] std::string dump_hex(std::span<const std::byte> bytes, std::uint32_t columns = 16);

}  // namespace knotwork
