#include "knotwork/obj.hpp"

#include <charconv>
#include <sstream>
#include <unordered_map>

namespace knotwork {
namespace {

struct ObjIndex {
  int position = 0;
  int texcoord = 0;
  int normal = 0;
};

struct ObjIndexHash {
  std::size_t operator()(const ObjIndex& index) const {
    std::size_t seed = static_cast<std::size_t>(index.position) * 73856093u;
    seed ^= static_cast<std::size_t>(index.texcoord) * 19349663u;
    seed ^= static_cast<std::size_t>(index.normal) * 83492791u;
    return seed;
  }
};

bool operator==(ObjIndex lhs, ObjIndex rhs) {
  return lhs.position == rhs.position && lhs.texcoord == rhs.texcoord && lhs.normal == rhs.normal;
}

std::vector<std::string_view> split_ws(std::string_view line) {
  std::vector<std::string_view> out;
  while (!line.empty()) {
    while (!line.empty() && (line.front() == ' ' || line.front() == '\t' || line.front() == '\r')) {
      line.remove_prefix(1);
    }
    if (line.empty()) {
      break;
    }
    const std::size_t next = line.find_first_of(" \t\r");
    out.push_back(next == std::string_view::npos ? line : line.substr(0, next));
    if (next == std::string_view::npos) {
      break;
    }
    line.remove_prefix(next + 1);
  }
  return out;
}

bool parse_float(std::string_view value, float& out) {
  const auto* first = value.data();
  const auto* last = first + value.size();
  const auto result = std::from_chars(first, last, out);
  return result.ec == std::errc{} && result.ptr == last;
}

bool parse_int(std::string_view value, int& out) {
  const auto* first = value.data();
  const auto* last = first + value.size();
  const auto result = std::from_chars(first, last, out);
  return result.ec == std::errc{} && result.ptr == last;
}

std::vector<std::string_view> split_slash(std::string_view value) {
  std::vector<std::string_view> out;
  while (true) {
    const std::size_t next = value.find('/');
    out.push_back(next == std::string_view::npos ? value : value.substr(0, next));
    if (next == std::string_view::npos) {
      break;
    }
    value.remove_prefix(next + 1);
  }
  return out;
}

bool parse_obj_index(std::string_view value, ObjIndex& out) {
  const auto parts = split_slash(value);
  if (parts.empty() || parts[0].empty() || !parse_int(parts[0], out.position)) {
    return false;
  }
  if (parts.size() > 1 && !parts[1].empty() && !parse_int(parts[1], out.texcoord)) {
    return false;
  }
  if (parts.size() > 2 && !parts[2].empty() && !parse_int(parts[2], out.normal)) {
    return false;
  }
  return true;
}

std::uint32_t resolve_index(int obj_index, std::size_t size) {
  if (obj_index > 0) {
    return static_cast<std::uint32_t>(obj_index - 1);
  }
  if (obj_index < 0) {
    return static_cast<std::uint32_t>(static_cast<int>(size) + obj_index);
  }
  return kNoIndex;
}

void issue(ObjLoadResult& result, std::uint32_t line, std::string message) {
  result.issues.push_back({line, std::move(message)});
}

}  // namespace

ObjLoadResult load_obj(std::string_view text, ObjOptions options) {
  ObjLoadResult result;
  std::vector<Vec3> positions;
  std::vector<Vec3> normals;
  std::vector<Vec2> texcoords;
  std::unordered_map<ObjIndex, std::uint32_t, ObjIndexHash> remap;
  std::istringstream input{std::string(text)};
  std::string line;
  std::uint32_t line_no = 0;

  const auto add_vertex = [&](ObjIndex index, std::uint32_t line_number) -> std::optional<std::uint32_t> {
    const auto found = remap.find(index);
    if (found != remap.end()) {
      return found->second;
    }
    const std::uint32_t position_index = resolve_index(index.position, positions.size());
    if (position_index >= positions.size()) {
      issue(result, line_number, "face references a missing position");
      return std::nullopt;
    }
    Vertex vertex;
    vertex.position = positions[position_index];
    if (index.normal != 0) {
      const std::uint32_t normal_index = resolve_index(index.normal, normals.size());
      if (normal_index >= normals.size()) {
        issue(result, line_number, "face references a missing normal");
        return std::nullopt;
      }
      vertex.normal = normals[normal_index];
    }
    if (index.texcoord != 0) {
      const std::uint32_t texcoord_index = resolve_index(index.texcoord, texcoords.size());
      if (texcoord_index >= texcoords.size()) {
        issue(result, line_number, "face references a missing texcoord");
        return std::nullopt;
      }
      vertex.texcoord = texcoords[texcoord_index];
    }
    const std::uint32_t out_index = static_cast<std::uint32_t>(result.mesh.vertices.size());
    result.mesh.vertices.push_back(vertex);
    remap.emplace(index, out_index);
    return out_index;
  };

  while (std::getline(input, line)) {
    ++line_no;
    const auto parts = split_ws(line);
    if (parts.empty() || parts[0].starts_with("#")) {
      continue;
    }
    if (parts[0] == "o" || parts[0] == "g") {
      if (parts.size() > 1 && result.mesh.name.empty()) {
        result.mesh.name = std::string(parts[1]);
      }
    } else if (parts[0] == "v") {
      if (parts.size() < 4) {
        issue(result, line_no, "position has too few fields");
        continue;
      }
      Vec3 value;
      if (!parse_float(parts[1], value.x) || !parse_float(parts[2], value.y) ||
          !parse_float(parts[3], value.z)) {
        issue(result, line_no, "position contains invalid floats");
        continue;
      }
      positions.push_back(value);
    } else if (parts[0] == "vn") {
      if (parts.size() < 4) {
        issue(result, line_no, "normal has too few fields");
        continue;
      }
      Vec3 value;
      if (!parse_float(parts[1], value.x) || !parse_float(parts[2], value.y) ||
          !parse_float(parts[3], value.z)) {
        issue(result, line_no, "normal contains invalid floats");
        continue;
      }
      normals.push_back(normalize(value));
    } else if (parts[0] == "vt") {
      if (parts.size() < 3) {
        issue(result, line_no, "texcoord has too few fields");
        continue;
      }
      Vec2 value;
      if (!parse_float(parts[1], value.x) || !parse_float(parts[2], value.y)) {
        issue(result, line_no, "texcoord contains invalid floats");
        continue;
      }
      texcoords.push_back(value);
    } else if (parts[0] == "f") {
      if (parts.size() < 4) {
        issue(result, line_no, "face has fewer than three vertices");
        continue;
      }
      std::vector<std::uint32_t> face;
      for (std::size_t i = 1; i < parts.size(); ++i) {
        ObjIndex index;
        if (!parse_obj_index(parts[i], index)) {
          issue(result, line_no, "face index is malformed");
          face.clear();
          break;
        }
        auto vertex_index = add_vertex(index, line_no);
        if (!vertex_index) {
          face.clear();
          break;
        }
        face.push_back(*vertex_index);
      }
      if (face.size() < 3) {
        continue;
      }
      if (face.size() == 3) {
        result.mesh.triangles.push_back({face[0], face[1], face[2]});
      } else if (options.triangulate) {
        for (std::size_t i = 1; i + 1 < face.size(); ++i) {
          result.mesh.triangles.push_back({face[0], face[i], face[i + 1]});
        }
      } else {
        issue(result, line_no, "non-triangle face encountered with triangulation disabled");
      }
    } else if (!options.tolerate_unknown_lines) {
      issue(result, line_no, "unknown OBJ directive");
    }
  }

  if (result.mesh.name.empty()) {
    result.mesh.name = "obj";
  }
  if (options.recalculate_missing_normals) {
    bool missing = false;
    for (const Vertex& vertex : result.mesh.vertices) {
      missing = missing || length(vertex.normal) <= 0.000001f;
    }
    if (missing) {
      recalculate_normals(result.mesh);
    }
  }
  if (options.recalculate_tangents) {
    recalculate_tangents(result.mesh);
  }
  return result;
}

std::string save_obj(const MeshAsset& mesh) {
  std::ostringstream out;
  out << "o " << (mesh.name.empty() ? "mesh" : mesh.name) << "\n";
  for (const Vertex& vertex : mesh.vertices) {
    out << "v " << vertex.position.x << " " << vertex.position.y << " " << vertex.position.z << "\n";
  }
  for (const Vertex& vertex : mesh.vertices) {
    out << "vt " << vertex.texcoord.x << " " << vertex.texcoord.y << "\n";
  }
  for (const Vertex& vertex : mesh.vertices) {
    out << "vn " << vertex.normal.x << " " << vertex.normal.y << " " << vertex.normal.z << "\n";
  }
  for (Triangle triangle : mesh.triangles) {
    const std::uint32_t a = triangle.a + 1;
    const std::uint32_t b = triangle.b + 1;
    const std::uint32_t c = triangle.c + 1;
    out << "f " << a << "/" << a << "/" << a << " " << b << "/" << b << "/" << b << " " << c
        << "/" << c << "/" << c << "\n";
  }
  return out.str();
}

std::string save_obj_with_material(const MeshAsset& mesh, std::string_view material_name) {
  std::ostringstream out;
  out << "mtllib " << material_name << ".mtl\n";
  out << "usemtl " << material_name << "\n";
  out << save_obj(mesh);
  return out.str();
}

bool obj_has_errors(const ObjLoadResult& result) {
  return !result.issues.empty();
}

}  // namespace knotwork
