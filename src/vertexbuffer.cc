#include "common.h"

#include "GLee.h"
#include <GLFW/glfw3.h>

#include "vertex.h"
#include "vertexbuffer.h"

#define USE_VBO 1

VertexBuffer::VertexBuffer() :
  dirty(true),
  vbo(0),
  verts()
{
}

VertexBuffer::VertexBuffer(const std::vector<Vertex> &verts) :
  dirty(true),
  vbo(0),
  verts(verts)
{
}

VertexBuffer::~VertexBuffer() {
#if USE_VBO
  if (this->vbo) {
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &this->vbo);
  }
#endif
}

void
VertexBuffer::Clear() {
  this->verts.clear();
  this->dirty = true;
}

size_t
VertexBuffer::Add(const Vertex &vert) {
  this->verts.push_back(vert);
  this->dirty = true;
  return this->verts.size()-1;
}

size_t
VertexBuffer::Add(const std::vector<Vertex> &verts) {
  for (auto &v:verts) this->verts.push_back(v);
  return this->verts.size()-1;
}

void
VertexBuffer::DrawTriangles(size_t first, size_t count) {
  if (count == 0) count = this->verts.size() - first;
  if (count == 0) return;

#if USE_VBO
  if (this->dirty) {
    if (!this->vbo) glGenBuffers(1, &this->vbo);
    if (!this->vbo) Log("%04x\n", glGetError());
    glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex)*(this->verts.size()), &this->verts[0], GL_STATIC_DRAW);
  } else {
    glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
  }
  glInterleavedArrays(GL_T2F_C4F_N3F_V3F,  sizeof(Vertex), nullptr);
#else
  glInterleavedArrays(GL_T2F_C4F_N3F_V3F,  sizeof(Vertex), &this->verts[0]);
#endif

  glDrawArrays(GL_TRIANGLES, first, count);
  this->dirty = false;
}

void
VertexBuffer::DrawQuads(size_t first, size_t count) {
  if (count == 0) count = this->verts.size() - first;
  if (count == 0) return;

#if USE_VBO
  if (this->dirty) {
    if (!this->vbo) glGenBuffers(1, &this->vbo);
    if (!this->vbo) Log("%04x\n", glGetError());
    glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex)*(this->verts.size()), &this->verts[0], GL_STATIC_DRAW);
  } else {
    glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
  }
  glInterleavedArrays(GL_T2F_C4F_N3F_V3F,  sizeof(Vertex), nullptr);
#else
  glInterleavedArrays(GL_T2F_C4F_N3F_V3F,  sizeof(Vertex), &this->verts[0]);
#endif

  glDrawArrays(GL_QUADS, first, count);
  this->dirty = false;
}
