#ifndef BARFOOS_UTIL_H
#define BARFOOS_UTIL_H

#include "common.h"

#include "vector3.h"

FILE *openAsset(const std::string &name);
std::string loadAssetAsString(const std::string &name);
std::vector <std::string> findAssets(const std::string &type);

time_t getFileChangeTime(const std::string &name);

float Wave(float x, float z, float t, float a = 0.2);

std::vector<std::string> Tokenize(const char *line);
size_t ParseSidesMask(const std::string &str);

int saveImage(const std::string &fileName, size_t w, size_t h, const uint8_t *rgb);

template<class T>
struct Smooth {

  Smooth(float f, const T &v = T() ) : 
    target(v),
    current(v),
    f(f)
  {}

  struct Smooth<T> &operator=(T value) {
    this->target = value;
    return *this;
  }
  
  void Update(float dt) {
    if (dt > 1.0 / this->f) {
      this->SnapTo(this->target);
    } else {
      this->current = this->current + (this->target - this->current) * dt * this->f;
    }
  }
  
  void SnapTo(T value) {
    this->target  = value;
    this->current = value;
  }

  T operator+(const T &o) const { return this->current + o; }
  T operator-(const T &o) const { return this->current - o; }
  
  operator const T&()     const { return this->current; }
  operator T()                  { return this->current; }
  
  const T &GetValue()     const { return this->current; }
  const T &GetTarget()    const { return this->target;  }
  
private:

  T target, current;
  float f;
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

#endif

