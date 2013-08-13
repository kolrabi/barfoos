#ifndef BARFOOS_UTIL_H
#define BARFOOS_UTIL_H

#include "common.h"

#include "vector3.h"

// asset management
FILE *openAsset(const std::string &name);
std::string loadAssetAsString(const std::string &name);
std::vector <std::string> findAssets(const std::string &type);

// file management
FILE *createUserFile(const std::string &name);
FILE *openUserFile(const std::string &name);
time_t getFileChangeTime(const std::string &name);


float Wave(float x, float z, float t, float a = 0.2);

std::vector<std::string> Tokenize(const char *line);
size_t ParseSidesMask(const std::string &str);

int saveImage(const std::string &fileName, size_t w, size_t h, const uint8_t *rgb);

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

#include <sstream>

template<class T> 
std::string ToString(const T &v) {
  std::stringstream str;
  str << v;
  return str.str();
}

class Regular {
public:
  Regular() : 
    interval(0.0),
    func(),
    t(0.0)
    {}

  Regular(float interval, std::function<void()> func) :
    interval(interval),
    func(func),
    t(0.0)
  {}
  
  void Update(float deltaT) {
    t += deltaT;
    while(t > interval) {
      t -= interval;
      func();
    }
  }

private:

  float interval;
  std::function<void()> func;
  float t;
};

#endif

