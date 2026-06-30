#pragma once

#include "knotwork/format.hpp"

#include <string>

namespace knotwork {

[[nodiscard]] std::string scene_to_json(const Scene& scene);

}  // namespace knotwork
