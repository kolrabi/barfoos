#ifndef BARFOOS_VECTOR3_H
#define BARFOOS_VECTOR3_H

#include "space.h"

struct Vector3 {
  float x,y,z;

  Vector3() : x(0), y(0), z(0) {}
  Vector3(float x, float y, float z) : x(x), y(y), z(z) {}
  
  Vector3(Side side) {
    switch(side) {
      case Side::Right:    *this = Vector3( 1, 0, 0); return;
      case Side::Left:     *this = Vector3(-1, 0, 0); return;
      case Side::Up:       *this = Vector3( 0, 1, 0); return;
      case Side::Down:     *this = Vector3( 0,-1, 0); return;
      case Side::Forward:  *this = Vector3( 0, 0, 1); return;
      case Side::Backward: *this = Vector3( 0, 0,-1); return;
      case Side::InvalidSide: 
      default:
        *this = Vector3();
    }
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
};

static inline std::ostream & operator<< (std::ostream &out, const Vector3 &v) {
  out << "[" << v.x << ", " << v.y << ", " << v.z << "]";
  return out;
}

#endif
