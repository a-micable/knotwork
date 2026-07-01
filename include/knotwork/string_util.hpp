#pragma once

#include <span>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace knotwork {

[[nodiscard]] std::string trim(std::string_view value);
[[nodiscard]] std::vector<std::string> split_lines(std::string_view value);
[[nodiscard]] std::vector<std::string> split_words_copy(std::string_view value);
[[nodiscard]] std::string join(std::span<const std::string> values, std::string_view separator);
[[nodiscard]] std::string slugify(std::string_view value);
[[nodiscard]] bool iequals(std::string_view a, std::string_view b);
[[nodiscard]] bool starts_with_ignore_case(std::string_view value, std::string_view prefix);
[[nodiscard]] std::string replace_all(std::string value, std::string_view from, std::string_view to);
[[nodiscard]] std::string repeat(std::string_view value, std::uint32_t count);
[[nodiscard]] std::string indent(std::string_view value, std::uint32_t spaces);
[[nodiscard]] std::string prefix_lines(std::string_view value, std::string_view prefix);
[[nodiscard]] std::vector<std::string> wrap_words(std::string_view value, std::uint32_t width);
[[nodiscard]] std::string to_lower(std::string_view value);
[[nodiscard]] std::string to_upper(std::string_view value);
[[nodiscard]] bool contains_ignore_case(std::string_view value, std::string_view needle);
[[nodiscard]] bool parse_bool(std::string_view value, bool& out);
[[nodiscard]] bool parse_u32(std::string_view value, std::uint32_t& out);
[[nodiscard]] std::string ensure_suffix(std::string value, std::string_view suffix);
[[nodiscard]] std::string common_prefix(std::span<const std::string> values);
[[nodiscard]] std::size_t edit_distance(std::string_view a, std::string_view b);
[[nodiscard]] std::string number_lines(std::string_view value, std::uint32_t first_line = 1);
[[nodiscard]] std::string pad_left(std::string value, std::size_t width, char fill = ' ');
[[nodiscard]] std::string pad_right(std::string value, std::size_t width, char fill = ' ');
[[nodiscard]] std::string truncate(std::string value, std::size_t width);

}  // namespace knotwork
