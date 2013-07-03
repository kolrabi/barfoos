#ifndef BARFOOS_VERTEX_H
#define BARFOOS_VERTEX_H

#include "icolor.h"

// GL_T2F_C4F_N3F_V3F
struct Vertex {
  float uv[2];
  float rgb[4];
  float n[3];
  float xyz[3];

  Vertex(const Vector3 &v, const IColor &c, float uu, float vv) {
    xyz[0] = v.x; xyz[1] = v.y; xyz[2] = v.z;
    rgb[0] = c.r/255.0; rgb[1] = c.g/255.0; rgb[2] = c.b/255.0; rgb[3] = 1.0;
    uv[0] = uu; uv[1] = vv;
    n[0] = n[1] = n[2] = 0;
  }
  
  Vertex(const Vector3 &v, const IColor &c, float uu, float vv, const Vector3 &norm) {
    xyz[0] = v.x; xyz[1] = v.y; xyz[2] = v.z;
    rgb[0] = c.r/255.0; rgb[1] = c.g/255.0; rgb[2] = c.b/255.0; rgb[3] = 1.0;
    uv[0] = uu; uv[1] = vv;
    
    Vector3 nn = norm.Normalize();
    n[0] = nn.x;
    n[1] = nn.y;
    n[2] = nn.z;
  }
};

#endif

