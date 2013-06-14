#ifndef BARFOOS_COMMON_H
#define BARFOOS_COMMON_H

#include <config.h>

#include <cstdint>
#include <cstdlib>
#include <cmath>

#include <vector>
#include <string>
#include <map>

#include <iostream>
#include <memory>
#include <algorithm>
#include <sstream>

#include <functional>

#if __cplusplus < 201103L
#define final
#endif

#include "vector3.h"
#include "space.h"

#include "ivector3.h"

struct AABB {
  Vector3 center;
  Vector3 extents;

  bool Overlap(const AABB &o) const {
    return std::abs(center.x - o.center.x) <= (extents.x + o.extents.x) &&
           std::abs(center.y - o.center.y) <= (extents.y + o.extents.y) &&
           std::abs(center.z - o.center.z) <= (extents.z + o.extents.z);
  }

  inline Vector3 Min() const { return center - extents; }
  inline Vector3 Max() const { return center + extents; }

  bool PointInside(const Vector3 &p) const {
    if (p.x>center.x+extents.x) return false;
    if (p.y>center.y+extents.y) return false;
    if (p.z>center.z+extents.z) return false;
    if (p.x<center.x-extents.x) return false;
    if (p.y<center.y-extents.y) return false;
    if (p.z<center.z-extents.z) return false;
    return true;
  }

  bool Ray(const Vector3 &start, const Vector3 &dir, float &t, Vector3 &p) const {
    if (PointInside(start)) return false;

    float tnear = -INFINITY;
    float tfar  =  INFINITY;

    Vector3 tmin, tmax;
    tmin.x = (Min().x - start.x) / dir.x; 
    tmax.x = (Max().x - start.x) / dir.x;

    if (tmin.x > tmax.x) std::swap(tmin.x, tmax.x); 

    if (tmin.x > tnear) tnear = tmin.x;
    if (tmax.x < tfar ) tfar  = tmax.x;

    if (tnear > tfar) {
      return false;
    }
 
    tmin.y = (Min().y - start.y) / dir.y;
    tmax.y = (Max().y - start.y) / dir.y;

    if (tmin.y > tmax.y) std::swap(tmin.y, tmax.y);
    
    if (tmin.y > tnear) tnear = tmin.y;
    if (tmax.y < tfar ) tfar  = tmax.y;
    
    if (tnear > tfar) {
      return false;
    }
    
    tmin.z = (Min().z - start.z) / dir.z;
    tmax.z = (Max().z - start.z) / dir.z;
    
    if (tmin.z > tmax.z) std::swap(tmin.z, tmax.z);

    if (tmin.z > tnear) tnear = tmin.z;
    if (tmax.z < tfar ) tfar  = tmax.z;
    
    if (tnear > tfar) {
      return false;
    }
    
    t = tnear;
    p = start + dir * t;
    return true;
  }
};

static inline std::ostream & operator<< (std::ostream &out, const AABB &aabb) {
  out << "{ " << aabb.center << " +/- " << aabb.extents << " }";
  return out;
}

struct IColor {
  int16_t r,g,b;
  
  IColor() : r(0), g(0), b(0) {}
  IColor(int r, int g, int b) : r(r), g(g), b(b) {}
 
  IColor Saturate(int max = 255) const {
    IColor c(*this);
    if (c.r < 0) c.r = 0; 
    if (c.r > max) c.r = max;
    if (c.g < 0) c.g = 0; 
    if (c.g > max) c.g = max;
    if (c.b < 0) c.b = 0; 
    if (c.b > max) c.b = max;
    return c;
  }

  bool IsBlack() const {
    IColor c = Saturate(); 
    return c.r == 0 && c.g == 0 && c.b == 0;
  }

  inline IColor Max(const IColor &o) const {
    return IColor( r > o.r ? r : o.r,
                   g > o.g ? g : o.g,
                   b > o.b ? b : o.b );
  }

  IColor operator-(int n) const {
    return IColor( r < n ? 0 : r-n, 
                   g < n ? 0 : g-n, 
                   b < n ? 0 : b-n );
  }
  
  IColor operator/(int n) const {
    return IColor( r/n, g/n, b/n); 
  }
  
  IColor operator*(int n) const {
    return IColor( r*n, g*n, b*n); 
  }
  
  IColor operator*(float f) const {
    return IColor( r*f, g*f, b*f); 
  }
  
  IColor operator+(const IColor &o) const {
    return IColor( r+o.r, g+o.g, b+o.b );
  }
  
  bool operator==(const IColor &o) const { 
    return r==o.r && g==o.g && b==o.b;
  }

  bool operator>=(const IColor &o) const { 
    return r>=o.r || g>=o.g || b>=o.b;
  }

  bool operator>(const IColor &o) const { 
    return r>o.r || g>o.g || b>o.b;
  }
  
  bool operator!=(const IColor &o) const { 
    return !(r==o.r && g==o.g && b==o.b);
  }
};

static inline std::ostream & operator<< (std::ostream &out, const IColor &c) {
  out << "{ " << c.r << " " << c.g << " " << c.b << " }";
  return out;
}

struct Vertex {
  float uv[2];
  float rgb[3];
  float xyz[3];

  Vertex(const Vector3 &v, const IColor &c, float uu, float vv) {
    xyz[0] = v.x; xyz[1] = v.y; xyz[2] = v.z;
    rgb[0] = c.r/255.0; rgb[1] = c.g/255.0; rgb[2] = c.b/255.0;
    uv[0] = uu; uv[1] = vv;
  }
};

struct Animation {
  size_t firstFrame;
  size_t frameCount;
  float fps;
  
  Animation(size_t firstFrame, size_t frameCount, float fps) : firstFrame(firstFrame), frameCount(frameCount), fps(fps) {}
};

#endif

