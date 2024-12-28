#pragma once

template <typename T>
struct Vec2 {
  T x;
  T y;

  Vec2():
    x(0),
    y(0) {}

  Vec2(T x, T y):
    x(x),
    y(y) {}

  Vec2(const T* data):
    x(data[0]),
    y(data[1]) {}

  operator T*() { return &x; }
  operator const T*() const { return &x; }

  Vec2 operator+(const Vec2& other) const { return Vec2(x + other.x, y + other.y); }
  Vec2 operator-(const Vec2& other) const { return Vec2(x - other.x, y - other.y); }
  Vec2 operator*(const Vec2& other) const { return Vec2(x * other.x, y * other.y); }
  Vec2 operator/(const Vec2& other) const { return Vec2(x / other.x, y / other.y); }

  Vec2 operator*(float scalar) const { return Vec2(x * scalar, y * scalar); }
  Vec2 operator/(float scalar) const { return Vec2(x / scalar, y / scalar); }
};

using Vec2i = Vec2<int>;
using Vec2f = Vec2<float>;

template <typename T>
struct Vec3 {
  T x;
  T y;
  T z;

  Vec3():
    x(0),
    y(0),
    z(0) {}

  Vec3(T x, T y, T z):
    x(x),
    y(y),
    z(z) {}

  Vec3(const T* data):
    x(data[0]),
    y(data[1]),
    z(data[2]) {}

  operator T*() { return &x; }
  operator const T*() const { return &x; }

  Vec3 operator+(const Vec3& other) const { return Vec3(x + other.x, y + other.y, z + other.z); }
  Vec3 operator-(const Vec3& other) const { return Vec3(x - other.x, y - other.y, z - other.z); }
  Vec3 operator*(const Vec3& other) const { return Vec3(x * other.x, y * other.y, z * other.z); }
  Vec3 operator/(const Vec3& other) const { return Vec3(x / other.x, y / other.y, z / other.z); }

  Vec3 operator*(float scalar) const { return Vec3(x * scalar, y * scalar, z * scalar); }
  Vec3 operator/(float scalar) const { return Vec3(x / scalar, y / scalar, z / scalar); }
};

using Vec3i = Vec3<int>;
using Vec3f = Vec3<float>;

template <typename T>
struct Vec4 {
  T x;
  T y;
  T z;
  T w;

  Vec4():
    x(0),
    y(0),
    z(0),
    w(0) {}

  Vec4(T x, T y, T z, T w):
    x(x),
    y(y),
    z(z),
    w(w) {}

  Vec4(const T* data):
    x(data[0]),
    y(data[1]),
    z(data[2]),
    w(data[3]) {}

  Vec2<T> xy() const { return {x, y}; }
  Vec2<T> xz() const { return {x, z}; }
  Vec2<T> yw() const { return {y, w}; }
  Vec2<T> zw() const { return {z, w}; }

  operator T*() { return &x; }
  operator const T*() const { return &x; }

  Vec4 operator+(const Vec4& other) const { return Vec4(x + other.x, y + other.y, z + other.z, w + other.w); }
  Vec4 operator-(const Vec4& other) const { return Vec4(x - other.x, y - other.y, z - other.z, w - other.w); }
  Vec4 operator*(const Vec4& other) const { return Vec4(x * other.x, y * other.y, z * other.z, w * other.w); }
  Vec4 operator/(const Vec4& other) const { return Vec4(x / other.x, y / other.y, z / other.z, w / other.w); }

  Vec4 operator*(float scalar) const { return Vec4(x * scalar, y * scalar, z * scalar, w * scalar); }
  Vec4 operator/(float scalar) const { return Vec4(x / scalar, y / scalar, z / scalar, w / scalar); }
};

using Vec4i = Vec4<int>;
using Vec4f = Vec4<float>;
