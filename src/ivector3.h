#ifndef BARFOOS_IVECTOR3_H
#define BARFOOS_IVECTOR3_H

#include "space.h"

#include "vector3.h"

struct IVector3 {
  size_t x,y,z;

  IVector3() : x(0), y(0), z(0) {}
  IVector3(size_t x, size_t y, size_t z) : x(x), y(y), z(z) {}
  IVector3(const Vector3 &v) : x(v.x), y(v.y), z(v.z) {}
  
  IVector3 operator+(const IVector3 &o) const {
    return IVector3(x+o.x, y+o.y, z+o.z);
  }

  IVector3 operator-(const IVector3 &o) const {
    return IVector3(x-o.x, y-o.y, z-o.z);
  }
  
  IVector3 operator-() const {
    return IVector3(-x, -y, -z);
  }
   
  IVector3 operator*(float f) const {
    return IVector3(x*f, y*f, z*f);
  }

  IVector3 operator/(float f) const {
    return IVector3(x/f, y/f, z/f);
  }

  bool operator==(const IVector3 &o) const {
    return x==o.x && y==o.y && z==o.z;
  }

  bool operator!=(const IVector3 &o) const {
    return !(x==o.x && y==o.y && z==o.z);
  }
  
  IVector3 operator[] (Side side) const {
    switch(side) {
       case Side::Right:    return IVector3(x+1,y,z);
       case Side::Left:     return IVector3(x-1,y,z);
       case Side::Up:       return IVector3(x,y+1,z);
       case Side::Down:     return IVector3(x,y-1,z);
       case Side::Forward:  return IVector3(x,y,z+1);
       case Side::Backward: return IVector3(x,y,z-1);
       case Side::InvalidSide: return *this;
    }
    return *this;
  }
  
  operator Vector3() const {
    return Vector3(x,y,z);
  }
  
  void For(std::function<void (const IVector3 &)> func) const {
    for (size_t z = 0; z<this->z; z++) {
      for (size_t y = 0; y<this->y; y++) {
        for (size_t x = 0; x<this->x; x++) {
          func(IVector3(x,y,z));
        }
      }
    }
  }
};

static inline std::ostream & operator<< (std::ostream &out, const IVector3 &v) {
  out << "[" << v.x << ", " << v.y << ", " << v.z << "]";
  return out;
}

#endif

