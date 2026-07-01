#pragma once

#include "knotwork/scene.hpp"

namespace knotwork {

enum class Unit {
  Millimeter,
  Centimeter,
  Meter,
  Kilometer,
  Inch,
  Foot,
};

[[nodiscard]] float meters_per_unit(Unit unit);
[[nodiscard]] const char* unit_name(Unit unit);
[[nodiscard]] float convert_length(float value, Unit from, Unit to);
[[nodiscard]] Vec3 convert_length(Vec3 value, Unit from, Unit to);
void scale_scene_units(Scene& scene, Unit from, Unit to);

}  // namespace knotwork
