#include "knotwork/table.hpp"

#include <iomanip>
#include <sstream>

namespace knotwork {
namespace {

std::vector<std::size_t> column_widths(const TextTable& table) {
  std::vector<std::size_t> widths(table.headers.size(), 0);
  for (std::size_t i = 0; i < table.headers.size(); ++i) {
    widths[i] = table.headers[i].size();
  }
  for (const auto& row : table.rows) {
    if (row.size() > widths.size()) {
      widths.resize(row.size(), 0);
    }
    for (std::size_t i = 0; i < row.size(); ++i) {
      widths[i] = std::max(widths[i], row[i].size());
    }
  }
  return widths;
}

void render_separator(std::ostringstream& out, const std::vector<std::size_t>& widths) {
  out << "+";
  for (std::size_t width : widths) {
    out << std::string(width + 2, '-') << "+";
  }
  out << "\n";
}

void render_row(std::ostringstream& out, const std::vector<std::string>& row, const std::vector<std::size_t>& widths) {
  out << "|";
  for (std::size_t i = 0; i < widths.size(); ++i) {
    const std::string value = i < row.size() ? row[i] : "";
    out << " " << std::left << std::setw(static_cast<int>(widths[i])) << value << " |";
  }
  out << "\n";
}

}  // namespace

void add_row(TextTable& table, std::vector<std::string> row) {
  table.rows.push_back(std::move(row));
}

std::string render_table(const TextTable& table) {
  const std::vector<std::size_t> widths = column_widths(table);
  std::ostringstream out;
  render_separator(out, widths);
  render_row(out, table.headers, widths);
  render_separator(out, widths);
  for (const auto& row : table.rows) {
    render_row(out, row, widths);
  }
  render_separator(out, widths);
  return out.str();
}

TextTable scene_node_table(const Scene& scene) {
  TextTable table;
  table.headers = {"index", "id", "name", "kind", "parent"};
  for (std::uint32_t i = 0; i < scene.nodes.size(); ++i) {
    const Node& node = scene.nodes[i];
    add_row(table,
            {std::to_string(i),
             std::to_string(node.id),
             std::string(scene.string_at(node.name).value_or("")),
             node_kind_name(node.kind),
             node.parent == kNoIndex ? "-" : std::to_string(node.parent)});
  }
  return table;
}

TextTable asset_table(const AssetLibrary& assets) {
  TextTable table;
  table.headers = {"handle", "name", "vertices", "triangles"};
  for (MeshHandle handle : assets.handles()) {
    const MeshAsset* mesh = assets.get(handle);
    if (mesh == nullptr) {
      continue;
    }
    add_row(table,
            {std::to_string(handle.index),
             mesh->name,
             std::to_string(mesh->vertices.size()),
             std::to_string(mesh->triangles.size())});
  }
  return table;
}

}  // namespace knotwork
