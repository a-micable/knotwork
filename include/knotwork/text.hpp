#pragma once

#include "knotwork/format.hpp"

#include <string>
#include <string_view>

namespace knotwork {

[[nodiscard]] std::string scene_to_text(const Scene& scene);
[[nodiscard]] Result<Scene> scene_from_text(std::string_view text);

}  // namespace knotwork
