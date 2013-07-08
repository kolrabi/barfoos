#ifndef BARFOOS_COMMON_H
#define BARFOOS_COMMON_H

#include <config.h>

#include <cstdint>
#include <cinttypes>
#include <cstdlib>
#include <cstring>
#include <cmath>

#include <vector>
#include <list>
#include <string>
#include <map>

#include <iostream>
#include <memory>
#include <algorithm>
#include <sstream>

#include <functional>

#if __cplusplus < 201103L
#define final
#define override

#ifndef PRIu64
#define PRIu64 "I64u"
#endif

#endif

#include "vector3.h"
#include "space.h"
#include "ivector3.h"

struct IColor;

//                 x  x  x  x
//   8  0          x  x  x  x
//   4  1  5       x  x  x  x
//   6  2  7       x  x  x  x
//      3          x  x  x  x
//                 x  x  x  x

/** Enum to identify inventory slots. */
enum class InventorySlot {
  Helmet     =  0, 
  Armor      =  1, 
  Greaves    =  2, 
  Boots      =  3,
  LeftHand   =  4, 
  RightHand  =  5,
  LeftRing   =  6, 
  RightRing  =  7,
  Amulet     =  8, 
  
  Reserved9  =  9,
  Reserved10 = 10,
  Reserved11 = 11,
  Reserved12 = 12,
  Reserved13 = 13,
  Reserved14 = 14,
  Reserved15 = 15,

  Backpack   = 16
  // ...
};

/** An axis aligned bounding box. */
struct AABB {
  /** Center of the box. */
  Vector3 center;
  
  /** Half the size of the box. */
  Vector3 extents;

  /** Test if another AABB overlaps the box. 
   * @param o AABB to check against.
   * @return true if @a this and @a o overlap.
   */
  bool Overlap(const AABB &o) const {
    return std::abs(center.x - o.center.x) <= (extents.x + o.extents.x) &&
           std::abs(center.y - o.center.y) <= (extents.y + o.extents.y) &&
           std::abs(center.z - o.center.z) <= (extents.z + o.extents.z);
  }

  /** Get the minimum vector of the box. */
  inline Vector3 Min() const { return center - extents; }
  
  /** Get the maximum vector of the box. */
  inline Vector3 Max() const { return center + extents; }

  /** Check if a point is inside the box. */
  bool PointInside(const Vector3 &p) const {
    if (p.x>center.x+extents.x) return false;
    if (p.y>center.y+extents.y) return false;
    if (p.z>center.z+extents.z) return false;
    if (p.x<center.x-extents.x) return false;
    if (p.y<center.y-extents.y) return false;
    if (p.z<center.z-extents.z) return false;
    return true;
  }
  
  /** Generate corner vertices for the box.
    * Also generates vertices for the sides.
    * @param[out] verts Where to store the vertices.
    * @TODO automate/optimize generation for a given aabb size
    * @TODO also cache these
    */
  void GetVertices(std::vector<Vector3> &verts) const {
    verts.push_back(Vector3(-extents.x, -extents.y, -extents.z));
    verts.push_back(Vector3(-extents.x, -extents.y,  extents.z));
    verts.push_back(Vector3( extents.x, -extents.y, -extents.z));
    verts.push_back(Vector3( extents.x, -extents.y,  extents.z));

    verts.push_back(Vector3(-extents.x, -extents.y/2, -extents.z));
    verts.push_back(Vector3(-extents.x, -extents.y/2,  extents.z));
    verts.push_back(Vector3( extents.x, -extents.y/2, -extents.z));
    verts.push_back(Vector3( extents.x, -extents.y/2,  extents.z));

    verts.push_back(Vector3(-extents.x,               0, -extents.z));
    verts.push_back(Vector3(-extents.x,               0,  extents.z));
    verts.push_back(Vector3( extents.x,               0, -extents.z));
    verts.push_back(Vector3( extents.x,               0,  extents.z));

    verts.push_back(Vector3(-extents.x,  extents.y/2, -extents.z));
    verts.push_back(Vector3(-extents.x,  extents.y/2,  extents.z));
    verts.push_back(Vector3( extents.x,  extents.y/2, -extents.z));
    verts.push_back(Vector3( extents.x,  extents.y/2,  extents.z));

    verts.push_back(Vector3(-extents.x,  extents.y, -extents.z));
    verts.push_back(Vector3(-extents.x,  extents.y,  extents.z));
    verts.push_back(Vector3( extents.x,  extents.y, -extents.z));
    verts.push_back(Vector3( extents.x,  extents.y,  extents.z));
  }

  /** Do a raycast against the box.
    * @param[in] start Start point of the ray.
    * @param[in] dir Normalized direction of the ray.
    * @param[out] t The distance of the hit if any.
    * @param[out] p The position of the hit if any.
    * @return true if ray hits the box.
    */
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
  out << "{" << aabb.center << " +/- " << aabb.extents << "}";
  return out;
}

/** Animation information. */
struct Animation {
  /** Frame index where the animation begins. */
  size_t firstFrame;

  /** Total number of frames in the animation. */
  size_t frameCount;
  
  /** Frames per second. */
  float fps;
  
  Animation(size_t firstFrame, size_t frameCount, float fps) : firstFrame(firstFrame), frameCount(frameCount), fps(fps) {}
};

/** A 2d point. */
struct Point {
  /** X coordinate. */
  int x;
  
  /** Y coordinate. */
  int y;

  Point() : x(0), y(0) {}  
  Point(int x, int y) : x(x), y(y) {}

  Point operator +(const Point &o) const {
    return Point(x+o.x, y+o.y);
  }

  Point operator -(const Point &o) const {
    return Point(x-o.x, y-o.y);
  }
};

static inline std::ostream & operator<< (std::ostream &out, const Point &p) {
  out << "{" << p.x << ":" << p.y << "}";
  return out;
}

/** A texture. */
struct Texture {
  Texture();
  Texture(Texture &&);
  ~Texture();
  
  /** OpenGL Handle of the texture. */
  unsigned int handle;
  
  /** Size of the texture. */
  Point size;
};

/** A sprite. */
struct Sprite {
  /** Texture to use when drawing the sprite. */
  const Texture *texture = nullptr;
  
  /** Sprite width. */
  float width = 1.0;
  
  /** Sprite height. */
  float height = 1.0;
  
  /** Horizontal offset used when drawing. */
  float offsetX = 0.0;
  
  /** Vertical offset used when drawing. */
  float offsetY = 0.0;
  
  /** Is this to be drawn as a vertical billboard? */
  bool vertical = false;
  
  /** Total number of frames in texture. */
  size_t totalFrames = 1;
  
  /** Current animation frame. */
  size_t currentFrame = 0;
  
  /** Current animation time. */
  float t = 0;
  
  /** Active animation index. */
  size_t currentAnimation = 0;
  
  /** All animations. */
  std::vector<Animation> animations;
  
  /** Update the sprite.
    * Advances animation time and frame. Resets animation to 0 when
    * last frame of animation reached.
    * @param deltaT Amount by which to advance the sprite.
    */
  void Update(float deltaT) {
    if (this->animations.size() > 0) {
      const Animation &anim = this->animations[currentAnimation];
      t += anim.fps * deltaT;
      currentFrame = t;
      if (t > anim.frameCount + anim.firstFrame) {
        currentAnimation = 0;
        t = t - currentFrame + this->animations[0].firstFrame;
      	currentFrame = t;
      }
    }
  }
  
};

/** A 2d rectangle. */
struct Rect {
  /** Top left corner of rectangle. */
  Point pos;
  
  /** Size of rectangle. */
  Point size;
  
  Rect() {} 
  Rect(const Point &pos, const Point &size) : pos(pos), size(size) {}

  /** Check if a point is inside the rectangle. 
    * @param p Point to test.
    * @return true if @a p is inside the rectangle.
    */
  bool IsInside(const Point &p) const {    
    return p.x >= pos.x && p.x < pos.x+size.x &&
           p.y >= pos.y && p.y < pos.y+size.y;
  }

  /** Adjust rectangle so that it encloses a point.
    * Won't change the rectangle if point already lies inside.
    * @param o Point to use.
    * @return The updated rectangle.
    */
  Rect &operator +=(const Point &o) {
    if (o.x < pos.x) {
      size.x += pos.x-o.x;
      pos.x = o.x;
    }
    if (o.y < pos.y) {
      size.y += pos.y-o.y;
      pos.y = o.y;
    }
    if (o.x > pos.x+size.x) {
      size.x += pos.x+size.x-o.x;
    }
    if (o.y > pos.y) {
      size.y += pos.y+size.y-o.y;
    }
    return *this;
  }
};

static inline std::ostream & operator<< (std::ostream &out, const Rect &r) {
  out << "{ " << r.pos << ":" << r.size << " }";
  return out;
}

template<class T> 
class temp_ptr {
public:
  temp_ptr(T *ptr) : ptr(ptr), moved(false) {}
  temp_ptr(const temp_ptr<T> &o) = delete;
  temp_ptr(temp_ptr<T> &&o) {
    if (o.moved) {
      ptr = nullptr;
    } else {      
      ptr = o.ptr;
    }
    moved = true;
    o.ptr = nullptr;
  }
  T* operator -> () { return ptr; }
  const T* operator -> () const { return ptr; }
  
  operator bool() const { return ptr != nullptr; }
  bool operator == (const temp_ptr<T> &o) {
    return o.ptr == this->ptr;
  }
  bool operator == (const T *o) {
    return o == this->ptr;
  }
  
private:
  T *ptr;
  bool moved;
};

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

/** Simple profiling class. */
class Profile {
public:

  Profile(const char *func, const char *file, int line);
  ~Profile();
  
  static void Dump();
  
private:
  
  std::string name;
  unsigned long long startTick;
};

#define PROFILE() Profile __profile(__PRETTY_FUNCTION__, __FILE__, __LINE__)

#endif

