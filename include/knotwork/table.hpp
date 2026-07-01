#pragma once

#include "knotwork/asset_library.hpp"

#include <string>
#include <vector>

namespace knotwork {

struct TextTable {
  std::vector<std::string> headers;
  std::vector<std::vector<std::string>> rows;
};

void add_row(TextTable& table, std::vector<std::string> row);
[[nodiscard]] std::string render_table(const TextTable& table);
[[nodiscard]] TextTable scene_node_table(const Scene& scene);
[[nodiscard]] TextTable asset_table(const AssetLibrary& assets);

}  // namespace knotwork
