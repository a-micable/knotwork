#include "knotwork/string_util.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <sstream>

namespace knotwork {

std::string trim(std::string_view value) {
  while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front()))) {
    value.remove_prefix(1);
  }
  while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back()))) {
    value.remove_suffix(1);
  }
  return std::string(value);
}

std::vector<std::string> split_lines(std::string_view value) {
  std::vector<std::string> lines;
  while (!value.empty()) {
    const std::size_t next = value.find('\n');
    std::string_view line = next == std::string_view::npos ? value : value.substr(0, next);
    if (!line.empty() && line.back() == '\r') {
      line.remove_suffix(1);
    }
    lines.emplace_back(line);
    if (next == std::string_view::npos) {
      break;
    }
    value.remove_prefix(next + 1);
  }
  return lines;
}

std::vector<std::string> split_words_copy(std::string_view value) {
  std::vector<std::string> words;
  while (!value.empty()) {
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front()))) {
      value.remove_prefix(1);
    }
    if (value.empty()) {
      break;
    }
    const std::size_t next = value.find_first_of(" \t\r\n");
    words.emplace_back(next == std::string_view::npos ? value : value.substr(0, next));
    if (next == std::string_view::npos) {
      break;
    }
    value.remove_prefix(next + 1);
  }
  return words;
}

std::string join(std::span<const std::string> values, std::string_view separator) {
  std::ostringstream out;
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i != 0) {
      out << separator;
    }
    out << values[i];
  }
  return out.str();
}

std::string slugify(std::string_view value) {
  std::string out;
  bool dash = false;
  for (char ch : value) {
    const unsigned char uch = static_cast<unsigned char>(ch);
    if (std::isalnum(uch)) {
      out.push_back(static_cast<char>(std::tolower(uch)));
      dash = false;
    } else if (!dash && !out.empty()) {
      out.push_back('-');
      dash = true;
    }
  }
  if (!out.empty() && out.back() == '-') {
    out.pop_back();
  }
  return out;
}

bool iequals(std::string_view a, std::string_view b) {
  if (a.size() != b.size()) {
    return false;
  }
  for (std::size_t i = 0; i < a.size(); ++i) {
    if (std::tolower(static_cast<unsigned char>(a[i])) !=
        std::tolower(static_cast<unsigned char>(b[i]))) {
      return false;
    }
  }
  return true;
}

bool starts_with_ignore_case(std::string_view value, std::string_view prefix) {
  if (prefix.size() > value.size()) {
    return false;
  }
  return iequals(value.substr(0, prefix.size()), prefix);
}

std::string replace_all(std::string value, std::string_view from, std::string_view to) {
  if (from.empty()) {
    return value;
  }
  std::size_t pos = 0;
  while ((pos = value.find(from, pos)) != std::string::npos) {
    value.replace(pos, from.size(), to);
    pos += to.size();
  }
  return value;
}

std::string repeat(std::string_view value, std::uint32_t count) {
  std::string out;
  out.reserve(value.size() * count);
  for (std::uint32_t i = 0; i < count; ++i) {
    out.append(value);
  }
  return out;
}

std::string indent(std::string_view value, std::uint32_t spaces) {
  return prefix_lines(value, repeat(" ", spaces));
}

std::string prefix_lines(std::string_view value, std::string_view prefix) {
  std::ostringstream out;
  const std::vector<std::string> lines = split_lines(value);
  for (const std::string& line : lines) {
    out << prefix << line << "\n";
  }
  return out.str();
}

std::vector<std::string> wrap_words(std::string_view value, std::uint32_t width) {
  std::vector<std::string> lines;
  std::vector<std::string> words = split_words_copy(value);
  width = std::max<std::uint32_t>(width, 1);
  std::string current;
  for (const std::string& word : words) {
    if (current.empty()) {
      current = word;
      continue;
    }
    if (current.size() + 1 + word.size() > width) {
      lines.push_back(current);
      current = word;
    } else {
      current += " ";
      current += word;
    }
  }
  if (!current.empty()) {
    lines.push_back(current);
  }
  return lines;
}

std::string to_lower(std::string_view value) {
  std::string out;
  out.reserve(value.size());
  for (char ch : value) {
    out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
  }
  return out;
}

std::string to_upper(std::string_view value) {
  std::string out;
  out.reserve(value.size());
  for (char ch : value) {
    out.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
  }
  return out;
}

bool contains_ignore_case(std::string_view value, std::string_view needle) {
  return to_lower(value).find(to_lower(needle)) != std::string::npos;
}

bool parse_bool(std::string_view value, bool& out) {
  const std::string trimmed = trim(value);
  if (iequals(trimmed, "true") || trimmed == "1" || iequals(trimmed, "yes") || iequals(trimmed, "on")) {
    out = true;
    return true;
  }
  if (iequals(trimmed, "false") || trimmed == "0" || iequals(trimmed, "no") || iequals(trimmed, "off")) {
    out = false;
    return true;
  }
  return false;
}

bool parse_u32(std::string_view value, std::uint32_t& out) {
  const std::string trimmed = trim(value);
  const auto result = std::from_chars(trimmed.data(), trimmed.data() + trimmed.size(), out);
  return result.ec == std::errc{} && result.ptr == trimmed.data() + trimmed.size();
}

std::string ensure_suffix(std::string value, std::string_view suffix) {
  if (value.size() >= suffix.size() &&
      std::string_view(value).substr(value.size() - suffix.size()) == suffix) {
    return value;
  }
  value.append(suffix);
  return value;
}

std::string common_prefix(std::span<const std::string> values) {
  if (values.empty()) {
    return {};
  }
  std::string prefix = values.front();
  for (std::size_t i = 1; i < values.size(); ++i) {
    std::size_t keep = 0;
    while (keep < prefix.size() && keep < values[i].size() && prefix[keep] == values[i][keep]) {
      ++keep;
    }
    prefix.resize(keep);
    if (prefix.empty()) {
      break;
    }
  }
  return prefix;
}

std::size_t edit_distance(std::string_view a, std::string_view b) {
  std::vector<std::size_t> previous(b.size() + 1);
  std::vector<std::size_t> current(b.size() + 1);
  for (std::size_t j = 0; j <= b.size(); ++j) {
    previous[j] = j;
  }
  for (std::size_t i = 1; i <= a.size(); ++i) {
    current[0] = i;
    for (std::size_t j = 1; j <= b.size(); ++j) {
      const std::size_t substitution = previous[j - 1] + (a[i - 1] == b[j - 1] ? 0 : 1);
      const std::size_t insertion = current[j - 1] + 1;
      const std::size_t deletion = previous[j] + 1;
      current[j] = std::min({substitution, insertion, deletion});
    }
    previous.swap(current);
  }
  return previous[b.size()];
}

std::string number_lines(std::string_view value, std::uint32_t first_line) {
  std::ostringstream out;
  for (const std::string& line : split_lines(value)) {
    out << first_line++ << ": " << line << "\n";
  }
  return out.str();
}

std::string pad_left(std::string value, std::size_t width, char fill) {
  if (value.size() >= width) {
    return value;
  }
  return std::string(width - value.size(), fill) + value;
}

std::string pad_right(std::string value, std::size_t width, char fill) {
  if (value.size() >= width) {
    return value;
  }
  value.append(width - value.size(), fill);
  return value;
}

std::string truncate(std::string value, std::size_t width) {
  if (value.size() <= width) {
    return value;
  }
  if (width <= 3) {
    value.resize(width);
    return value;
  }
  value.resize(width - 3);
  value += "...";
  return value;
}

}  // namespace knotwork
