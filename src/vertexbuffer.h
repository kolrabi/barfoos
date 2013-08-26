#ifndef BARFOOS_VERTEXBUFFER_H
#define BARFOOS_VERTEXBUFFER_H

#include "common.h"

class VertexBuffer final {
public:

  VertexBuffer();
  VertexBuffer(const std::vector<Vertex> &verts);

  ~VertexBuffer();

  void Clear();
  size_t Add(const Vertex &vert);
  size_t Add(const std::vector<Vertex> &verts);

private:

  bool dirty;
  unsigned int vbo;
  std::vector<Vertex> verts;

  void DrawTriangles(size_t first, size_t count);
  void DrawQuads(size_t first, size_t count);

  friend class Gfx;
};

#endif

