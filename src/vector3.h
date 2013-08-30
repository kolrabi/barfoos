#ifndef BARFOOS_VECTOR3_H
#define BARFOOS_VECTOR3_H

#include "common.h"
#include "space.h"

struct Vector3 {
  float x,y,z;

  Vector3()                           : x(0), y(0), z(0) {}
  Vector3(const float &f)             : x(f), y(f), z(f) {}
  Vector3(float x, float y, float z)  : x(x), y(y), z(z) {}

  Vector3(Side side)                  :
    x( side == Side::Right   ? 1 : (side == Side::Left     ? -1 : 0) ),
    y( side == Side::Up      ? 1 : (side == Side::Down     ? -1 : 0) ),
    z( side == Side::Forward ? 1 : (side == Side::Backward ? -1 : 0) ) {
  }

  bool operator ==(const Vector3 &o) const {
    return x == o.x && y == o.y && z == o.z;
  }

  bool operator !=(const Vector3 &o) const {
    return x != o.x || y != o.y || z != o.z;
  }

  Vector3 operator +(const Vector3 &o) const {
    return Vector3(x+o.x, y+o.y, z+o.z);
  }

  Vector3 operator -(const Vector3 &o) const {
    return Vector3(x-o.x, y-o.y, z-o.z);
  }

  Vector3 operator +(const float &o) const {
    return Vector3(x+o, y+o, z+o);
  }

  Vector3 operator -(const float &o) const {
    return Vector3(x-o, y-o, z-o);
  }

  Vector3 operator -() const {
    return Vector3(-x, -y, -z);
  }

  Vector3 operator *(const float f) const {
    return Vector3(x*f, y*f, z*f);
  }

  Vector3 operator /(const float f) const {
    return Vector3(x/f, y/f, z/f);
  }

  Vector3 operator *(const Vector3 &o) const {
    return Vector3(x*o.x, y*o.y, z*o.z);
  }

  Vector3 operator /(const Vector3 &o) const {
    return Vector3(x/o.x, y/o.y, z/o.z);
  }

  Vector3 Min(const Vector3 &o) const {
    return Vector3( std::min(x,o.x), std::min(y,o.y), std::min(z,o.z) );
  }

  Vector3 Max(const Vector3 &o) const {
    return Vector3( std::max(x,o.x), std::max(y,o.y), std::max(z,o.z) );
  }

  float GetSquareMag() const {
    return x*x + y*y + z*z;
  }

  float GetMag() const {
    return std::sqrt(x*x + y*y + z*z);
  }

  Vector3 EulerToVector() const {
    return Vector3( cos(x)*cos(y), sin(y), sin(x)*cos(y) );
  }

  Vector3 Cross(const Vector3 &o) const {
    return Vector3( o.z * y - o.y * z, o.x * z - o.z * x, o.y * x - o.x * y);
  }

  float Dot(const Vector3 &o) const {
    return x*o.x + y*o.y + z*o.z;
  }

  Vector3 Normalize() const {
    return *this * (1.0/GetMag());
  }

  Vector3 Horiz() const {
    return Vector3(x, 0, z);
  }

  Vector3 Vert() const {
    return Vector3(0, y, 0);
  }

  static Vector3 Normal(const Vector3 &a, const Vector3 &b, const Vector3 &c) {
    return (b-a).Cross(c-a).Normalize();
  }

  Vector3 XYZ() const { return *this; }
  Vector3 XZY() const { return Vector3(x,z,y); }
  Vector3 YXZ() const { return Vector3(y,x,z); }
  Vector3 YZX() const { return Vector3(y,z,x); }
  Vector3 ZXY() const { return Vector3(z,x,y); }
  Vector3 ZYX() const { return Vector3(z,y,x); }

  /** Raycast against a triangle.
    * @param[in] tri The three points of the triangle.
    * @param[in] start The ray start point.
    * @param[in] dir The normalized ray direction.
    * @param[out] t The time of hit if any.
    * @param[out] p The position of hit if any.
    * @return true if ray hit the triangle.
    */
  static inline bool 
  TriangleRay(const Vector3 *tri, const Vector3 &start, const Vector3 &dir, float &t, Vector3 &p) {
    Vector3 e1 = tri[1] - tri[0];
    Vector3 e2 = tri[2] - tri[0];
    
    Vector3 h = dir.Cross(e2);
    float a = e1.Dot(h);
    
    if (a < 0.00001) return false;
    
    float f = 1.0/a;
    
    Vector3 s = start-tri[0];
    float u = f * (s.Dot(h));
    if (u < 0.0 || u > 1.0) return false;
    
    Vector3 q = s.Cross(e1);
    float v = f * (dir.Normalize().Dot(q));
    if (v < 0.0 || v > 1.0) return false;
    
    float tt = f * e2.Dot(q);
    if (tt < 0.00001) return false;
    
    t = tt;
    p = start + dir * t;
    return true;
  }

  operator std::string() const;
};


#endif
