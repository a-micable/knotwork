#include "knotwork/package.hpp"
#include "knotwork/obj.hpp"

#include <cstring>
#include <sstream>

namespace knotwork {
namespace {

constexpr char kPackageMagic[8] = {'K', 'N', 'O', 'T', 'P', 'K', 'G', '\0'};
constexpr std::uint16_t kPackageVersion = 1;

struct PackageHeader {
  char magic[8]{};
  std::uint16_t version = 0;
  std::uint16_t reserved = 0;
  std::uint32_t entry_count = 0;
};

struct EntryHeader {
  std::uint8_t kind = 0;
  std::uint8_t reserved0 = 0;
  std::uint16_t reserved1 = 0;
  std::uint32_t name_size = 0;
  std::uint32_t payload_size = 0;
};

class ByteWriter {
 public:
  template <typename T>
  void write(const T& value) {
    const auto* first = reinterpret_cast<const std::byte*>(&value);
    bytes_.insert(bytes_.end(), first, first + sizeof(T));
  }

  void write_bytes(std::span<const std::byte> bytes) {
    bytes_.insert(bytes_.end(), bytes.begin(), bytes.end());
  }

  void write_text(std::string_view text) {
    const auto* first = reinterpret_cast<const std::byte*>(text.data());
    bytes_.insert(bytes_.end(), first, first + text.size());
  }

  std::vector<std::byte> take() {
    return std::move(bytes_);
  }

 private:
  std::vector<std::byte> bytes_;
};

class ByteReader {
 public:
  explicit ByteReader(std::span<const std::byte> bytes) : bytes_(bytes) {}

  template <typename T>
  bool read(T& value) {
    if (remaining() < sizeof(T)) {
      return false;
    }
    std::memcpy(&value, bytes_.data() + offset_, sizeof(T));
    offset_ += sizeof(T);
    return true;
  }

  bool read_text(std::string& out, std::uint32_t size) {
    if (remaining() < size) {
      return false;
    }
    out.assign(reinterpret_cast<const char*>(bytes_.data() + offset_), size);
    offset_ += size;
    return true;
  }

  bool read_bytes(std::vector<std::byte>& out, std::uint32_t size) {
    if (remaining() < size) {
      return false;
    }
    out.assign(bytes_.begin() + static_cast<std::ptrdiff_t>(offset_),
               bytes_.begin() + static_cast<std::ptrdiff_t>(offset_ + size));
    offset_ += size;
    return true;
  }

  std::size_t remaining() const {
    return bytes_.size() - offset_;
  }

 private:
  std::span<const std::byte> bytes_;
  std::size_t offset_ = 0;
};

SceneError package_error(std::string message) {
  return {SceneErrorCode::InvalidString, std::move(message)};
}

std::vector<std::byte> bytes_from_text(std::string_view text) {
  const auto* first = reinterpret_cast<const std::byte*>(text.data());
  return {first, first + text.size()};
}

}  // namespace

PackageEntry make_scene_entry(std::string name, const Scene& scene) {
  PackageEntry entry;
  entry.kind = PackageEntryKind::Scene;
  entry.name = std::move(name);
  entry.payload = save_scene(scene);
  return entry;
}

PackageEntry make_mesh_obj_entry(std::string name, const MeshAsset& mesh) {
  PackageEntry entry;
  entry.kind = PackageEntryKind::Mesh;
  entry.name = std::move(name);
  const std::string obj = save_obj(mesh);
  entry.payload = bytes_from_text(obj);
  return entry;
}

PackageEntry make_text_entry(std::string name, std::string_view text) {
  PackageEntry entry;
  entry.kind = PackageEntryKind::Text;
  entry.name = std::move(name);
  entry.payload = bytes_from_text(text);
  return entry;
}

std::vector<std::byte> save_package(const Package& package) {
  ByteWriter writer;
  PackageHeader header;
  std::memcpy(header.magic, kPackageMagic, sizeof(header.magic));
  header.version = kPackageVersion;
  header.entry_count = static_cast<std::uint32_t>(package.entries.size());
  writer.write(header);
  for (const PackageEntry& entry : package.entries) {
    EntryHeader item;
    item.kind = static_cast<std::uint8_t>(entry.kind);
    item.name_size = static_cast<std::uint32_t>(entry.name.size());
    item.payload_size = static_cast<std::uint32_t>(entry.payload.size());
    writer.write(item);
    writer.write_text(entry.name);
    writer.write_bytes(entry.payload);
  }
  return writer.take();
}

Result<Package> load_package(std::span<const std::byte> bytes) {
  ByteReader reader(bytes);
  PackageHeader header;
  if (!reader.read(header)) {
    return package_error("package is too short for a header");
  }
  if (std::memcmp(header.magic, kPackageMagic, sizeof(header.magic)) != 0) {
    return package_error("package magic is invalid");
  }
  if (header.version != kPackageVersion) {
    return SceneError{SceneErrorCode::UnsupportedVersion, "package version is unsupported"};
  }
  Package package;
  package.entries.reserve(header.entry_count);
  for (std::uint32_t i = 0; i < header.entry_count; ++i) {
    EntryHeader item;
    if (!reader.read(item)) {
      return package_error("package ended while reading an entry header");
    }
    if (item.kind < static_cast<std::uint8_t>(PackageEntryKind::Scene) ||
        item.kind > static_cast<std::uint8_t>(PackageEntryKind::Binary)) {
      return SceneError{SceneErrorCode::InvalidEnum, "package entry kind is invalid"};
    }
    PackageEntry entry;
    entry.kind = static_cast<PackageEntryKind>(item.kind);
    if (!reader.read_text(entry.name, item.name_size)) {
      return package_error("package ended while reading an entry name");
    }
    if (!reader.read_bytes(entry.payload, item.payload_size)) {
      return package_error("package ended while reading an entry payload");
    }
    package.entries.push_back(std::move(entry));
  }
  return package;
}

std::optional<PackageEntry> find_entry(const Package& package, std::string_view name) {
  for (const PackageEntry& entry : package.entries) {
    if (entry.name == name) {
      return entry;
    }
  }
  return std::nullopt;
}

std::string package_manifest(const Package& package) {
  std::ostringstream out;
  out << "entries=" << package.entries.size() << "\n";
  for (const PackageEntry& entry : package.entries) {
    out << entry.name << " kind=" << static_cast<int>(entry.kind) << " bytes=" << entry.payload.size()
        << "\n";
  }
  return out.str();
}

}  // namespace knotwork
