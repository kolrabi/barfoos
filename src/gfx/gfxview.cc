#include "GLee.h"

#include "common.h"

#include "gfx/gfx.h"
#include "gfx/gfxview.h"
#include "gfx/shader.h"
#include "math/aabb.h"
#include "math/matrix4.h"

GfxView::GfxView(Gfx &gfx) :
  gfx(gfx),
  projectionMatrix(),
  viewMatrix(),
  modelMatrixStack(1),
  textureMatrix()
{}

void GfxView::Look(const Vector3 &pos, const Vector3 &forward, float fovY, const Vector3 &up) {
  float aspect = gfx.GetScreen().GetAspect();

  this->pos = pos;
  this->forward = forward.Normalize();
  this->right = forward.Cross(up).Normalize();
  this->up = right.Cross(forward).Normalize();

  if (fovY > 0.0) {
    this->projectionMatrix = Matrix4::Perspective(fovY, aspect, 0.0015f, 64.0f);
  } else {
    this->projectionMatrix = Matrix4::Ortho(fovY*aspect, -fovY*aspect, fovY, -fovY, 0.0015f, 64.0f);
  }

  this->viewMatrix = Matrix4::LookFrom(pos, forward, up);
  glEnable(GL_DEPTH_TEST);
}

void GfxView::GUI() {
  this->projectionMatrix = Matrix4();

  const Point &ssize = gfx.GetScreen().GetVirtualSize();
  this->viewMatrix =
    Matrix4::Scale(    Vector3(  2.0/ssize.x, -2.0/ssize.y, 1)) *
    Matrix4::Translate(Vector3( -0.5*ssize.x, -0.5*ssize.y, 0));

  this->modelMatrixStack.back() = Matrix4();
  glDisable(GL_DEPTH_TEST);
}

void GfxView::Push() {
  this->modelMatrixStack.push_back(this->modelMatrixStack.back());
}

void GfxView::Pop() {
  this->modelMatrixStack.pop_back();
  if (this->modelMatrixStack.empty()) this->modelMatrixStack.push_back(Matrix4());
}

void GfxView::Translate(const Vector3 &p) {
  this->modelMatrixStack.back() = this->modelMatrixStack.back() * Matrix4::Translate(p);
}

void GfxView::Scale(const Vector3 &p) {
  this->modelMatrixStack.back() = this->modelMatrixStack.back() * Matrix4::Scale(p);
}

void GfxView::Rotate(float angle, const Vector3 &p)  {
  this->modelMatrixStack.back() = this->modelMatrixStack.back() * Matrix4::Rotate(angle, p);
}

void GfxView::Billboard(bool flip, bool vertical) {
  Matrix4 &m = this->modelMatrixStack.back();

  Vector3 dir   = (m * Vector3()) - this->pos;
  Vector3 up    = vertical ? Vector3(0,1,0) : this->up;
  Vector3 right = dir.Normalize().Cross(up).Normalize();
  Vector3 fwd   = right.Cross(up).Normalize();

  m(0,0) = flip ? -right.x : right.x;
  m(0,1) = flip ? -right.y : right.y;
  m(0,2) = flip ? -right.z : right.z;

  m(1,0) = up.x;
  m(1,1) = up.y;
  m(1,2) = up.z;

  m(2,0) = fwd.x;
  m(2,1) = fwd.y;
  m(2,2) = fwd.z;

  m(0,3) = 0.0;
  m(1,3) = 0.0;
  m(2,3) = 0.0;
  m(3,3) = 1.0;
}

void
GfxView::AlignY(const Vector3 &axis) {
  Matrix4 &m = this->modelMatrixStack.back();

  Vector3 dir   = (m * Vector3()) - this->pos;
  Vector3 up    = axis.Normalize();
  Vector3 right = dir.Normalize().Cross(up).Normalize();
  Vector3 fwd   = right.Cross(axis).Normalize();

  m(0,0) = right.x;
  m(0,1) = right.y;
  m(0,2) = right.z;

  m(1,0) = up.x;
  m(1,1) = up.y;
  m(1,2) = up.z;

  m(2,0) = fwd.x;
  m(2,1) = fwd.y;
  m(2,2) = fwd.z;

  m(0,3) = 0.0;
  m(1,3) = 0.0;
  m(2,3) = 0.0;
  m(3,3) = 1.0;
}

void GfxView::SetUniforms(const std::shared_ptr<Shader> &shader) const {
  Matrix4 matModelView = this->viewMatrix * this->modelMatrixStack.back();
  if (gfx.UseFixedFunction()) {
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(this->projectionMatrix.m);
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(matModelView.m);
    glMatrixMode(GL_TEXTURE);
    glLoadMatrixf(this->textureMatrix.m);
  } else {
    shader->Uniform("u_matProjection",    this->projectionMatrix);
    shader->Uniform("u_matModelView",     matModelView);
    shader->Uniform("u_matView",          this->viewMatrix);
    shader->Uniform("u_matInvModelView",  matModelView.Inverse());
    shader->Uniform("u_matTexture",       this->textureMatrix);
    shader->Uniform("u_matNormal",        matModelView.Mat3().Inverse().Transpose());
  }
}

Vector3 GfxView::WorldToScreen(const Vector3 &p) const {
  Vector4 pp = this->projectionMatrix * (this->viewMatrix*Vector4(p));
  return pp.DivW();
}

Vector3 GfxView::WorldToScreen(const Vector3 &p, bool &visible) const {
  Vector3 p2 = (this->projectionMatrix * (this->viewMatrix*Vector4(p))).DivW();
  visible = (p2.x > -1 && p2.x < 1 && p2.y > -1 && p2.y < 1 && p2.z > -0.001);
  return p2;
}

bool GfxView::IsPointVisible(const Vector3 &p) const {
  Vector3 p2 = WorldToScreen(p);
  return p2.x > -1 && p2.x < 1 && p2.y > -1 && p2.y < 1 && p2.z > -0.001;
}

bool GfxView::IsAABBVisible(const AABB &aabb) const {
  if (IsPointVisible(aabb.center)) return true;

  static const AABB unitAABB(Vector3(1.0, 1.0, 1.0));
  AABB screenAABB;
  for (uint8_t i=0; i<8; i++) {
    bool visible = false;
    Vector3 v = WorldToScreen(aabb.GetCorner(i), visible);
    if (visible) return true;
    screenAABB.Combine(v);
  }
  return screenAABB.Overlap(unitAABB);
}
