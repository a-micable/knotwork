#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <ostream>
#include <string>

namespace knotwork {

struct Vec3 {
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;

  [[nodiscard]] std::string to_string() const;
};

struct Quat {
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;
  float w = 1.0f;

  [[nodiscard]] float length_squared() const;
  [[nodiscard]] Quat normalized() const;
};

struct Mat4 {
  std::array<float, 16> m{};

  [[nodiscard]] static Mat4 identity();
  [[nodiscard]] static Mat4 translation(const Vec3& value);
  [[nodiscard]] static Mat4 scale(const Vec3& value);
  [[nodiscard]] static Mat4 rotation(const Quat& value);
  [[nodiscard]] static Mat4 compose(const Vec3& t, const Quat& r, const Vec3& s);

  [[nodiscard]] float at(int row, int column) const;
  float& at(int row, int column);
  [[nodiscard]] std::string to_string() const;
};

[[nodiscard]] Mat4 operator*(const Mat4& lhs, const Mat4& rhs);
[[nodiscard]] bool nearly_equal(float lhs, float rhs, float epsilon = 0.0001f);
[[nodiscard]] bool nearly_equal(const Mat4& lhs, const Mat4& rhs, float epsilon = 0.0001f);

std::ostream& operator<<(std::ostream& os, const Mat4& matrix);

}  // namespace knotwork
