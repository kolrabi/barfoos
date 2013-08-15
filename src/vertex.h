#ifndef BARFOOS_VERTEX_H
#define BARFOOS_VERTEX_H

#include "icolor.h"
#include "vector3.h"

// GL_T2F_C4F_N3F_V3F
struct Vertex {
  float uv[2];
  float rgb[4];
  float n[3];
  float xyz[3];
  
  Vertex() : 
    uv { 0, 0 },
    rgb{ 1, 1, 1, 1 },
    n  { 0, 0, 0 },
    xyz{ 0, 0, 0 }
  {}

  Vertex(const Vector3 &v, const IColor &c, float uu, float vv, const Vector3 &norm = Vector3()) :
    uv { uu, vv },
    rgb{ c.r/255.0f, c.g/255.0f,  c.b/255.0f, 1.0f },
    n  { norm.x, norm.y, norm.z },
    xyz{ v.x, v.y, v.z }
  {}
  
  void SetColor(const IColor &c) {
    rgb[0] = c.r/255.0f;
    rgb[1] = c.g/255.0f;
    rgb[2] = c.b/255.0f;
    rgb[3] = 1.0f;
  }
};

#endif

