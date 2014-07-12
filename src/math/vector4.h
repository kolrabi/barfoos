#ifndef BARFOOS_VECTOR4_H
#define BARFOOS_VECTOR4_H

#include "space.h"

struct Vector4 {
  float x,y,z,w;

  Vector4()                           : x(0), y(0), z(0), w(1) {}
  Vector4(const float &f)             : x(f), y(f), z(f), w(1) {}
  Vector4(float x, float y, float z)  : x(x), y(y), z(z), w(1) {}
  Vector4(float x, float y, float z, float w)  : x(x), y(y), z(z), w(w) {}
  
  Vector4(const Vector3 &v3, float w = 1.0) :
    x(v3.x), y(v3.y), z(v3.z), w(w) {}

  bool operator ==(const Vector4 &o) const {
    return x == o.x && y == o.y && z == o.z && w == o.w;
  }

  bool operator !=(const Vector4 &o) const {
    return x != o.x || y != o.y || z != o.z || w != o.w;
  }

  Vector4 operator +(const Vector4 &o) const {
    return Vector4(x+o.x, y+o.y, z+o.z, w+o.w);
  }

  Vector4 operator -(const Vector4 &o) const {
    return Vector4(x-o.x, y-o.y, z-o.z, w-o.w);
  }

  Vector4 operator +(const float &o) const {
    return Vector4(x+o, y+o, z+o);
  }

  Vector4 operator -(const float &o) const {
    return Vector4(x-o, y-o, z-o, w-o);
  }

  Vector4 operator -() const {
    return Vector4(-x, -y, -z, -w);
  }

  Vector4 operator *(const float f) const {
    return Vector4(x*f, y*f, z*f, w*f);
  }

  Vector4 operator /(const float f) const {
    return Vector4(x/f, y/f, z/f, w/f);
  }

  Vector4 operator *(const Vector4 &o) const {
    return Vector4(x*o.x, y*o.y, z*o.z, w*o.w);
  }

  Vector4 operator /(const Vector4 &o) const {
    return Vector4(x/o.x, y/o.y, z/o.z, w/o.w);
  }

  Vector4 Min(const Vector4 &o) const {
    return Vector4( std::min(x,o.x), std::min(y,o.y), std::min(z,o.z), std::min(w,o.w) );
  }

  Vector4 Max(const Vector4 &o) const {
    return Vector4( std::max(x,o.x), std::max(y,o.y), std::max(z,o.z), std::max(w,o.w) );
  }

  float GetSquareMag() const {
    return x*x + y*y + z*z + w*w;
  }

  float GetMag() const {
    return std::sqrt(x*x + y*y + z*z + w*w);
  }

  Vector4 Normalize() const {
    return *this * (1.0/GetMag());
  }
  
  Vector3 XYZ()  const { return Vector3(x,y,z);   }
  Vector3 DivW() const { return Vector3(x,y,z)/w; }
  
  operator std::string() const;
};


#endif
