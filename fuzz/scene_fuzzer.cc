#include "knotwork/flatten.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <span>
#include <vector>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
  const auto* bytes = reinterpret_cast<const std::byte*>(data);
  auto scene = knotwork::load_scene(std::span<const std::byte>(bytes, size));
  if (!scene.ok()) {
    return 0;
  }
  auto flattened = knotwork::flatten_scene(scene.value());
  if (!flattened.ok()) {
    return 0;
  }
  volatile std::size_t sink = flattened.value().size();
  (void)sink;
  return 0;
}

#ifdef KNOTWORK_STANDALONE_FUZZER
namespace {

std::vector<std::uint8_t> read_file(const std::filesystem::path& path) {
  std::ifstream input(path, std::ios::binary | std::ios::ate);
  if (!input) {
    return {};
  }
  const std::streamsize size = input.tellg();
  input.seekg(0, std::ios::beg);
  std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
  input.read(reinterpret_cast<char*>(bytes.data()), size);
  return bytes;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "usage: scene_fuzzer <scene-or-corpus-path>...\n";
    return 2;
  }
  for (int i = 1; i < argc; ++i) {
    const std::filesystem::path path(argv[i]);
    if (std::filesystem::is_directory(path)) {
      for (const auto& entry : std::filesystem::directory_iterator(path)) {
        if (!entry.is_regular_file()) {
          continue;
        }
        const auto bytes = read_file(entry.path());
        LLVMFuzzerTestOneInput(bytes.data(), bytes.size());
      }
    } else {
      const auto bytes = read_file(path);
      LLVMFuzzerTestOneInput(bytes.data(), bytes.size());
    }
  }
  return 0;
}
#endif
