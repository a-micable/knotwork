#include "knotwork/units.hpp"

namespace knotwork {

float meters_per_unit(Unit unit) {
  switch (unit) {
    case Unit::Millimeter:
      return 0.001f;
    case Unit::Centimeter:
      return 0.01f;
    case Unit::Meter:
      return 1.0f;
    case Unit::Kilometer:
      return 1000.0f;
    case Unit::Inch:
      return 0.0254f;
    case Unit::Foot:
      return 0.3048f;
  }
  return 1.0f;
}

const char* unit_name(Unit unit) {
  switch (unit) {
    case Unit::Millimeter:
      return "millimeter";
    case Unit::Centimeter:
      return "centimeter";
    case Unit::Meter:
      return "meter";
    case Unit::Kilometer:
      return "kilometer";
    case Unit::Inch:
      return "inch";
    case Unit::Foot:
      return "foot";
  }
  return "unknown";
}

float convert_length(float value, Unit from, Unit to) {
  return value * meters_per_unit(from) / meters_per_unit(to);
}

Vec3 convert_length(Vec3 value, Unit from, Unit to) {
  const float scale = meters_per_unit(from) / meters_per_unit(to);
  return {value.x * scale, value.y * scale, value.z * scale};
}

void scale_scene_units(Scene& scene, Unit from, Unit to) {
  const float scale = meters_per_unit(from) / meters_per_unit(to);
  for (Node& node : scene.nodes) {
    node.local.translation = {node.local.translation.x * scale,
                              node.local.translation.y * scale,
                              node.local.translation.z * scale};
  }
}

}  // namespace knotwork
