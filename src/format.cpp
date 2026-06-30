#include "knotwork/format.hpp"
#include "knotwork/validate.hpp"

#include <array>
#include <cstring>
#include <fstream>
#include <limits>
#include <span>

namespace knotwork {
namespace {

constexpr std::array<char, 8> kMagic{'K', 'N', 'O', 'T', 'S', 'C', 'N', '\0'};
constexpr std::uint16_t kVersion = 1;
constexpr std::uint32_t kMaxStrings = 65536;
constexpr std::uint32_t kMaxMaterials = 65536;
constexpr std::uint32_t kMaxNodes = 250000;
constexpr std::uint32_t kMaxStringBytes = 1024 * 1024;

struct Header {
  std::array<char, 8> magic{};
  std::uint16_t version = 0;
  std::uint16_t flags = 0;
  std::uint32_t string_count = 0;
  std::uint32_t material_count = 0;
  std::uint32_t node_count = 0;
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

  bool read_string(std::string& value) {
    std::uint32_t size = 0;
    if (!read(size) || size > kMaxStringBytes || remaining() < size) {
      return false;
    }
    value.assign(reinterpret_cast<const char*>(bytes_.data() + offset_), size);
    offset_ += size;
    return value.find('\0') == std::string::npos;
  }

  [[nodiscard]] std::size_t remaining() const {
    return bytes_.size() - offset_;
  }

 private:
  std::span<const std::byte> bytes_;
  std::size_t offset_ = 0;
};

class Writer {
 public:
  template <typename T>
  void write(const T& value) {
    const auto* first = reinterpret_cast<const std::byte*>(&value);
    bytes_.insert(bytes_.end(), first, first + sizeof(T));
  }

  void write_string(const std::string& value) {
    const auto size = static_cast<std::uint32_t>(value.size());
    write(size);
    const auto* first = reinterpret_cast<const std::byte*>(value.data());
    bytes_.insert(bytes_.end(), first, first + value.size());
  }

  [[nodiscard]] std::vector<std::byte> take() {
    return std::move(bytes_);
  }

 private:
  std::vector<std::byte> bytes_;
};

SceneError error(SceneErrorCode code, std::string message) {
  return {code, std::move(message)};
}

bool valid_kind(std::uint8_t value) {
  return value <= static_cast<std::uint8_t>(NodeKind::Instance);
}

bool valid_light_kind(std::uint8_t value) {
  return value <= static_cast<std::uint8_t>(LightKind::Spot);
}

}  // namespace

Result<Scene> load_scene(std::span<const std::byte> bytes) {
  Reader reader(bytes);
  Header header;
  if (!reader.read(header)) {
    return error(SceneErrorCode::ShortRead, "file is too short for scene header");
  }
  if (header.magic != kMagic) {
    return error(SceneErrorCode::BadMagic, "scene magic does not match KNOTSCN");
  }
  if (header.version != kVersion) {
    return error(SceneErrorCode::UnsupportedVersion, "scene version is not supported");
  }
  if (header.string_count > kMaxStrings || header.material_count > kMaxMaterials ||
      header.node_count > kMaxNodes) {
    return error(SceneErrorCode::CountLimitExceeded, "scene table count exceeds implementation limit");
  }

  Scene scene;
  scene.strings.reserve(header.string_count);
  scene.materials.reserve(header.material_count);
  scene.nodes.reserve(header.node_count);

  for (std::uint32_t i = 0; i < header.string_count; ++i) {
    std::string value;
    if (!reader.read_string(value)) {
      return error(SceneErrorCode::InvalidString, "string table contains an invalid entry");
    }
    scene.strings.push_back(std::move(value));
  }

  for (std::uint32_t i = 0; i < header.material_count; ++i) {
    Material material;
    if (!reader.read(material.name) || !reader.read(material.base_color) ||
        !reader.read(material.roughness) || !reader.read(material.metallic)) {
      return error(SceneErrorCode::ShortRead, "file ended while reading material table");
    }
    scene.materials.push_back(material);
  }

  for (std::uint32_t i = 0; i < header.node_count; ++i) {
    Node node;
    std::uint8_t kind = 0;
    std::uint8_t light_kind = 0;
    std::uint16_t reserved = 0;
    if (!reader.read(node.id) || !reader.read(node.name) || !reader.read(kind) ||
        !reader.read(light_kind) || !reader.read(reserved) || !reader.read(node.parent) ||
        !reader.read(node.material) || !reader.read(node.instance_target) ||
        !reader.read(node.local.translation) || !reader.read(node.local.rotation) ||
        !reader.read(node.local.scale) || !reader.read(node.light_intensity) ||
        !reader.read(node.camera_fov_y)) {
      return error(SceneErrorCode::ShortRead, "file ended while reading node table");
    }
    if (!valid_kind(kind) || !valid_light_kind(light_kind)) {
      return error(SceneErrorCode::InvalidEnum, "node table contains an invalid enum value");
    }
    node.kind = static_cast<NodeKind>(kind);
    node.light_kind = static_cast<LightKind>(light_kind);
    scene.nodes.push_back(node);
  }

  const SceneError validation = validate_scene(scene);
  if (validation.code != SceneErrorCode::None) {
    return validation;
  }
  return scene;
}

Result<Scene> load_scene_file(const std::filesystem::path& path) {
  std::ifstream input(path, std::ios::binary | std::ios::ate);
  if (!input) {
    return error(SceneErrorCode::Io, "could not open scene file");
  }
  const std::streamsize size = input.tellg();
  if (size < 0) {
    return error(SceneErrorCode::Io, "could not determine scene file size");
  }
  input.seekg(0, std::ios::beg);
  std::vector<std::byte> bytes(static_cast<std::size_t>(size));
  input.read(reinterpret_cast<char*>(bytes.data()), size);
  if (!input && size != 0) {
    return error(SceneErrorCode::Io, "could not read scene file");
  }
  return load_scene(bytes);
}

std::vector<std::byte> save_scene(const Scene& scene) {
  Writer writer;
  Header header;
  header.magic = kMagic;
  header.version = kVersion;
  header.string_count = static_cast<std::uint32_t>(scene.strings.size());
  header.material_count = static_cast<std::uint32_t>(scene.materials.size());
  header.node_count = static_cast<std::uint32_t>(scene.nodes.size());
  writer.write(header);

  for (const auto& value : scene.strings) {
    writer.write_string(value);
  }
  for (const Material& material : scene.materials) {
    writer.write(material.name);
    writer.write(material.base_color);
    writer.write(material.roughness);
    writer.write(material.metallic);
  }
  for (const Node& node : scene.nodes) {
    const auto kind = static_cast<std::uint8_t>(node.kind);
    const auto light_kind = static_cast<std::uint8_t>(node.light_kind);
    const std::uint16_t reserved = 0;
    writer.write(node.id);
    writer.write(node.name);
    writer.write(kind);
    writer.write(light_kind);
    writer.write(reserved);
    writer.write(node.parent);
    writer.write(node.material);
    writer.write(node.instance_target);
    writer.write(node.local.translation);
    writer.write(node.local.rotation);
    writer.write(node.local.scale);
    writer.write(node.light_intensity);
    writer.write(node.camera_fov_y);
  }
  return writer.take();
}

bool save_scene_file(const std::filesystem::path& path, const Scene& scene, SceneError& out_error) {
  const SceneError validation = validate_scene(scene);
  if (validation.code != SceneErrorCode::None) {
    out_error = validation;
    return false;
  }
  const auto bytes = save_scene(scene);
  std::ofstream output(path, std::ios::binary);
  if (!output) {
    out_error = error(SceneErrorCode::Io, "could not open output path");
    return false;
  }
  output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  if (!output) {
    out_error = error(SceneErrorCode::Io, "could not write scene file");
    return false;
  }
  out_error = {};
  return true;
}

const char* scene_error_name(SceneErrorCode code) {
  switch (code) {
    case SceneErrorCode::None:
      return "none";
    case SceneErrorCode::ShortRead:
      return "short-read";
    case SceneErrorCode::BadMagic:
      return "bad-magic";
    case SceneErrorCode::UnsupportedVersion:
      return "unsupported-version";
    case SceneErrorCode::CountLimitExceeded:
      return "count-limit-exceeded";
    case SceneErrorCode::InvalidString:
      return "invalid-string";
    case SceneErrorCode::InvalidEnum:
      return "invalid-enum";
    case SceneErrorCode::InvalidReference:
      return "invalid-reference";
    case SceneErrorCode::DuplicateNodeId:
      return "duplicate-node-id";
    case SceneErrorCode::Cycle:
      return "cycle";
    case SceneErrorCode::Io:
      return "io";
  }
  return "unknown";
}

}  // namespace knotwork
