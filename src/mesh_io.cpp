#include "knotwork/mesh_io.hpp"

#include <cstring>
#include <fstream>

namespace knotwork {
namespace {

constexpr char kMeshMagic[8] = {'K', 'N', 'O', 'T', 'M', 'S', 'H', '\0'};
constexpr std::uint16_t kMeshVersion = 1;
constexpr std::uint32_t kMaxMeshNameBytes = 1024 * 1024;
constexpr std::uint32_t kMaxMeshVertices = 1000000;
constexpr std::uint32_t kMaxMeshTriangles = 2000000;

struct MeshHeader {
  char magic[8]{};
  std::uint16_t version = 0;
  std::uint16_t reserved = 0;
  std::uint32_t name_size = 0;
  std::uint32_t vertex_count = 0;
  std::uint32_t triangle_count = 0;
  std::uint32_t material = kNoIndex;
};

class Writer {
 public:
  template <typename T>
  void write(const T& value) {
    const auto* first = reinterpret_cast<const std::byte*>(&value);
    bytes_.insert(bytes_.end(), first, first + sizeof(T));
  }
  void write_text(std::string_view value) {
    const auto* first = reinterpret_cast<const std::byte*>(value.data());
    bytes_.insert(bytes_.end(), first, first + value.size());
  }
  std::vector<std::byte> take() { return std::move(bytes_); }

 private:
  std::vector<std::byte> bytes_;
};

class Reader {
 public:
  explicit Reader(std::span<const std::byte> bytes) : bytes_(bytes) {}
  template <typename T>
  bool read(T& value) {
    if (remaining() < sizeof(T)) {
      return false;
    }
    std::memcpy(&value, bytes_.data() + offset_, sizeof(T));
    offset_ += sizeof(T);
    return true;
  }
  bool read_text(std::string& value, std::uint32_t size) {
    if (remaining() < size) {
      return false;
    }
    value.assign(reinterpret_cast<const char*>(bytes_.data() + offset_), size);
    offset_ += size;
    return true;
  }
  std::size_t remaining() const { return bytes_.size() - offset_; }

 private:
  std::span<const std::byte> bytes_;
  std::size_t offset_ = 0;
};

MeshIoError error(std::string message) {
  return {std::move(message)};
}

std::uint64_t fnv1a(std::uint64_t hash, const void* data, std::size_t size) {
  const auto* bytes = static_cast<const unsigned char*>(data);
  for (std::size_t i = 0; i < size; ++i) {
    hash ^= bytes[i];
    hash *= 1099511628211ull;
  }
  return hash;
}

bool count_fits_remaining(std::uint32_t count, std::size_t element_size, std::size_t remaining) {
  return count <= remaining / element_size;
}

}  // namespace

std::vector<std::byte> save_mesh_binary(const MeshAsset& mesh) {
  Writer writer;
  MeshHeader header;
  std::memcpy(header.magic, kMeshMagic, sizeof(header.magic));
  header.version = kMeshVersion;
  header.name_size = static_cast<std::uint32_t>(mesh.name.size());
  header.vertex_count = static_cast<std::uint32_t>(mesh.vertices.size());
  header.triangle_count = static_cast<std::uint32_t>(mesh.triangles.size());
  header.material = mesh.material;
  writer.write(header);
  writer.write_text(mesh.name);
  for (const Vertex& vertex : mesh.vertices) {
    writer.write(vertex);
  }
  for (Triangle triangle : mesh.triangles) {
    writer.write(triangle);
  }
  return writer.take();
}

MeshIoResult<MeshAsset> load_mesh_binary(std::span<const std::byte> bytes) {
  Reader reader(bytes);
  MeshHeader header;
  if (!reader.read(header)) {
    return error("mesh file is too short");
  }
  if (std::memcmp(header.magic, kMeshMagic, sizeof(header.magic)) != 0) {
    return error("mesh magic is invalid");
  }
  if (header.version != kMeshVersion) {
    return error("mesh version is unsupported");
  }
  if (header.name_size > kMaxMeshNameBytes || header.vertex_count > kMaxMeshVertices ||
      header.triangle_count > kMaxMeshTriangles) {
    return error("mesh table count exceeds implementation limit");
  }
  MeshAsset mesh;
  mesh.material = header.material;
  if (!reader.read_text(mesh.name, header.name_size)) {
    return error("mesh file ended while reading name");
  }
  if (!count_fits_remaining(header.vertex_count, sizeof(Vertex), reader.remaining())) {
    return error("mesh vertex table exceeds remaining input");
  }
  const std::size_t bytes_after_vertices =
      reader.remaining() - static_cast<std::size_t>(header.vertex_count) * sizeof(Vertex);
  if (!count_fits_remaining(header.triangle_count, sizeof(Triangle), bytes_after_vertices)) {
    return error("mesh triangle table exceeds remaining input");
  }
  mesh.vertices.resize(header.vertex_count);
  mesh.triangles.resize(header.triangle_count);
  for (Vertex& vertex : mesh.vertices) {
    if (!reader.read(vertex)) {
      return error("mesh file ended while reading vertices");
    }
  }
  for (Triangle& triangle : mesh.triangles) {
    if (!reader.read(triangle)) {
      return error("mesh file ended while reading triangles");
    }
  }
  const auto errors = validate_mesh(mesh);
  if (!errors.empty()) {
    return error(errors.front().message);
  }
  return mesh;
}

bool save_mesh_binary_file(const std::filesystem::path& path, const MeshAsset& mesh, MeshIoError& out_error) {
  const std::vector<std::byte> bytes = save_mesh_binary(mesh);
  std::ofstream output(path, std::ios::binary);
  if (!output) {
    out_error = error("could not open mesh output file");
    return false;
  }
  output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  if (!output) {
    out_error = error("could not write mesh file");
    return false;
  }
  out_error = {};
  return true;
}

MeshIoResult<MeshAsset> load_mesh_binary_file(const std::filesystem::path& path) {
  std::ifstream input(path, std::ios::binary | std::ios::ate);
  if (!input) {
    return error("could not open mesh input file");
  }
  const std::streamsize size = input.tellg();
  input.seekg(0, std::ios::beg);
  std::vector<std::byte> bytes(static_cast<std::size_t>(size));
  input.read(reinterpret_cast<char*>(bytes.data()), size);
  if (!input && size != 0) {
    return error("could not read mesh input file");
  }
  return load_mesh_binary(bytes);
}

std::uint64_t mesh_content_hash(const MeshAsset& mesh) {
  std::uint64_t hash = 1469598103934665603ull;
  hash = fnv1a(hash, mesh.name.data(), mesh.name.size());
  hash = fnv1a(hash, &mesh.material, sizeof(mesh.material));
  for (const Vertex& vertex : mesh.vertices) {
    hash = fnv1a(hash, &vertex, sizeof(vertex));
  }
  for (Triangle triangle : mesh.triangles) {
    hash = fnv1a(hash, &triangle, sizeof(triangle));
  }
  return hash;
}

}  // namespace knotwork
