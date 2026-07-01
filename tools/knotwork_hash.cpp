#include "knotwork/checksum.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>

namespace {

std::vector<std::byte> read_file(const std::filesystem::path& path) {
  std::ifstream input(path, std::ios::binary | std::ios::ate);
  if (!input) {
    return {};
  }
  const std::streamsize size = input.tellg();
  input.seekg(0, std::ios::beg);
  std::vector<std::byte> bytes(static_cast<std::size_t>(size));
  input.read(reinterpret_cast<char*>(bytes.data()), size);
  return bytes;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "usage: knotwork-hash <file>\n";
    return 2;
  }
  const std::vector<std::byte> bytes = read_file(argv[1]);
  if (bytes.empty()) {
    std::cerr << "could not read file or file is empty\n";
    return 1;
  }
  std::cout << knotwork::checksum_text(knotwork::checksums(bytes));
  return 0;
}
