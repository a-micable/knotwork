#include "knotwork/math.hpp"

#include <iomanip>
#include <sstream>

namespace knotwork {

std::string Vec3::to_string() const {
  std::ostringstream out;
  out << x << "," << y << "," << z;
  return out.str();
}

float Quat::length_squared() const {
  return x * x + y * y + z * z + w * w;
}

Quat Quat::normalized() const {
  const float len_sq = length_squared();
  if (len_sq <= 0.000001f) {
    return {};
  }
  const float inv_len = 1.0f / std::sqrt(len_sq);
  return {x * inv_len, y * inv_len, z * inv_len, w * inv_len};
}

Mat4 Mat4::identity() {
  Mat4 result;
  result.at(0, 0) = 1.0f;
  result.at(1, 1) = 1.0f;
  result.at(2, 2) = 1.0f;
  result.at(3, 3) = 1.0f;
  return result;
}

Mat4 Mat4::translation(const Vec3& value) {
  Mat4 result = identity();
  result.at(0, 3) = value.x;
  result.at(1, 3) = value.y;
  result.at(2, 3) = value.z;
  return result;
}

Mat4 Mat4::scale(const Vec3& value) {
  Mat4 result = identity();
  result.at(0, 0) = value.x;
  result.at(1, 1) = value.y;
  result.at(2, 2) = value.z;
  return result;
}

Mat4 Mat4::rotation(const Quat& value) {
  const Quat q = value.normalized();
  const float xx = q.x * q.x;
  const float yy = q.y * q.y;
  const float zz = q.z * q.z;
  const float xy = q.x * q.y;
  const float xz = q.x * q.z;
  const float yz = q.y * q.z;
  const float wx = q.w * q.x;
  const float wy = q.w * q.y;
  const float wz = q.w * q.z;

  Mat4 result = identity();
  result.at(0, 0) = 1.0f - 2.0f * (yy + zz);
  result.at(0, 1) = 2.0f * (xy - wz);
  result.at(0, 2) = 2.0f * (xz + wy);
  result.at(1, 0) = 2.0f * (xy + wz);
  result.at(1, 1) = 1.0f - 2.0f * (xx + zz);
  result.at(1, 2) = 2.0f * (yz - wx);
  result.at(2, 0) = 2.0f * (xz - wy);
  result.at(2, 1) = 2.0f * (yz + wx);
  result.at(2, 2) = 1.0f - 2.0f * (xx + yy);
  return result;
}

Mat4 Mat4::compose(const Vec3& t, const Quat& r, const Vec3& s) {
  return translation(t) * rotation(r) * scale(s);
}

float Mat4::at(int row, int column) const {
  return m[static_cast<std::size_t>(row * 4 + column)];
}

float& Mat4::at(int row, int column) {
  return m[static_cast<std::size_t>(row * 4 + column)];
}

std::string Mat4::to_string() const {
  std::ostringstream out;
  out << std::fixed << std::setprecision(4);
  for (int row = 0; row < 4; ++row) {
    if (row != 0) {
      out << " ";
    }
    out << "[";
    for (int column = 0; column < 4; ++column) {
      if (column != 0) {
        out << ", ";
      }
      out << at(row, column);
    }
    out << "]";
  }
  return out.str();
}

Mat4 operator*(const Mat4& lhs, const Mat4& rhs) {
  Mat4 result;
  for (int row = 0; row < 4; ++row) {
    for (int column = 0; column < 4; ++column) {
      float sum = 0.0f;
      for (int k = 0; k < 4; ++k) {
        sum += lhs.at(row, k) * rhs.at(k, column);
      }
      result.at(row, column) = sum;
    }
  }
  return result;
}

bool nearly_equal(float lhs, float rhs, float epsilon) {
  return std::fabs(lhs - rhs) <= epsilon;
}

bool nearly_equal(const Mat4& lhs, const Mat4& rhs, float epsilon) {
  for (std::size_t i = 0; i < lhs.m.size(); ++i) {
    if (!nearly_equal(lhs.m[i], rhs.m[i], epsilon)) {
      return false;
    }
  }
  return true;
}

std::ostream& operator<<(std::ostream& os, const Mat4& matrix) {
  os << matrix.to_string();
  return os;
}

}  // namespace knotwork
