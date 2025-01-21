#pragma once

struct Vec2i {
  int x;
  int y;

  Vec2i():
    x(0),
    y(0) {}

  Vec2i(int x, int y):
    x(x),
    y(y) {}

  operator int*() { return &x; }

  bool operator==(const Vec2i& other) const { return x == other.x && y == other.y; }
  bool operator!=(const Vec2i& other) const { return x != other.x || y != other.y; }
};

struct Vec2f {
  float x;
  float y;

  Vec2f():
    x(0),
    y(0) {}

  Vec2f(float x, float y):
    x(x),
    y(y) {}

  operator float*() { return &x; }

  Vec2f operator+(const Vec2f& other) const { return Vec2f(x + other.x, y + other.y); }
  Vec2f operator-(const Vec2f& other) const { return Vec2f(x - other.x, y - other.y); }
  Vec2f operator*(float scalar) const { return Vec2f(x * scalar, y * scalar); }
};

struct Vec3f {
  float x;
  float y;
  float z;

  Vec3f():
    x(0),
    y(0),
    z(0) {}

  Vec3f(float x, float y, float z):
    x(x),
    y(y),
    z(z) {}

  Vec3f operator+(const Vec3f& other) const { return Vec3f(x + other.x, y + other.y, z + other.z); }
  Vec3f operator-(const Vec3f& other) const { return Vec3f(x - other.x, y - other.y, z - other.z); }
  Vec3f operator*(float scalar) const { return Vec3f(x * scalar, y * scalar, z * scalar); }

  Vec3f& operator+=(const Vec3f& other) {
    x += other.x;
    y += other.y;
    z += other.z;
    return *this;
  }
  Vec3f& operator-=(const Vec3f& other) {
    x -= other.x;
    y -= other.y;
    z -= other.z;
    return *this;
  }
};

struct Vec4f {
  float x;
  float y;
  float z;
  float w;

  Vec4f():
    x(0),
    y(0),
    z(0),
    w(0) {}

  Vec4f(float x, float y, float z, float w):
    x(x),
    y(y),
    z(z),
    w(w) {}

  operator float*() { return &x; }
};
