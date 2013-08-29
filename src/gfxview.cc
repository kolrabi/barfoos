#include "GLee.h"

#include "gfx.h"
#include "gfxview.h"

#include "shader.h"
#include "matrix4.h"

#include "aabb.h"

void GfxView::Look(const Vector3 &pos, const Vector3 &forward, float fovY, const Vector3 &up) {
  float aspect = (float)gfx.viewportSize.x / (float)gfx.viewportSize.y;

  this->right = forward.Cross(up);

  if (fovY > 0.0) {
    this->projStack.back() = Matrix4::Perspective(fovY, aspect, 0.0015f, 64.0f);
  } else {
    this->projStack.back() = Matrix4::Ortho(fovY*aspect, -fovY*aspect, fovY, -fovY, 0.0015f, 64.0f);
  }

  this->viewStack.back() =
  this->modelViewStack.back() = Matrix4::LookFrom(pos, forward, up);
  glEnable(GL_DEPTH_TEST);
}

void GfxView::GUI() {
  this->projStack.back() = Matrix4();

  this->modelViewStack.back() =
    Matrix4::Scale(    Vector3(2.0/gfx.virtualScreenSize.x, -2.0/gfx.virtualScreenSize.y, 1)) *
    Matrix4::Translate(Vector3( -gfx.virtualScreenSize.x/2, -gfx.virtualScreenSize.y/2,   0));

  /*if (gfx.screenSize.x > 800 && gfx.screenSize.y > 600) {
    this->modelViewStack.back() = this->modelViewStack.back() * Matrix4::Scale(Vector3(2,2,1));
  }*/
  this->viewStack.back() = this->modelViewStack.back();
  glDisable(GL_DEPTH_TEST);
}

void GfxView::Push() {
  this->projStack.push_back(this->projStack.back());
  this->modelViewStack.push_back(this->modelViewStack.back());
  this->viewStack.push_back(this->viewStack.back());
  this->textureStack.push_back(this->textureStack.back());
}

void GfxView::Pop() {
  this->projStack.pop_back();
  this->modelViewStack.pop_back();
  this->viewStack.pop_back();
  this->textureStack.pop_back();

  if (projStack.empty())      projStack.push_back(Matrix4());
  if (modelViewStack.empty()) modelViewStack.push_back(Matrix4());
  if (viewStack.empty()) viewStack.push_back(Matrix4());
  if (textureStack.empty())   textureStack.push_back(Matrix4());
}

void GfxView::Translate(const Vector3 &p) {
  this->modelViewStack.back() = this->modelViewStack.back() * Matrix4::Translate(p);
}

void GfxView::Scale(const Vector3 &p) {
  this->modelViewStack.back() = this->modelViewStack.back() * Matrix4::Scale(p);
}

void GfxView::Rotate(float angle, const Vector3 &p)  {
  this->modelViewStack.back() = this->modelViewStack.back() * Matrix4::Rotate(angle, p);
}

void GfxView::Billboard(bool flip, bool vertical) {
  this->modelViewStack.back()(0,0) = flip?-1:1;
  this->modelViewStack.back()(0,1) = 0;
  this->modelViewStack.back()(0,2) = 0;

  if (!vertical) {
    this->modelViewStack.back()(1,0) = 0;
    this->modelViewStack.back()(1,1) = 1;
    this->modelViewStack.back()(1,2) = 0;
  }

  this->modelViewStack.back()(2,0) = 0;
  this->modelViewStack.back()(2,1) = 0;
  this->modelViewStack.back()(2,2) = 1;
}

void GfxView::SetUniforms(const std::shared_ptr<Shader> &shader) const {
  if (gfx.UseFixedFunction()) {
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(this->projStack.back().m);
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(this->modelViewStack.back().m);
    glMatrixMode(GL_TEXTURE);
    glLoadMatrixf(this->textureStack.back().m);
  } else {
    shader->Uniform("u_matProjection",    this->projStack.back());
    shader->Uniform("u_matModelView",     this->modelViewStack.back());
    shader->Uniform("u_matView",          this->viewStack.back());
    shader->Uniform("u_matInvModelView",  this->modelViewStack.back().Inverse());
    shader->Uniform("u_matTexture",       this->textureStack.back());
    shader->Uniform("u_matNormal",        this->modelViewStack.back().Mat3().Inverse().Transpose());
  }
}

Vector3 GfxView::WorldToScreen(const Vector3 &p) const {
  Vector4 pp = this->projStack.back() * (this->viewStack.back()*Vector4(p));
  return pp.DivW();
}

Vector3 GfxView::WorldToScreen(const Vector3 &p, bool &visible) const {
  Vector3 p2 = (this->projStack.back() * (this->viewStack.back()*Vector4(p))).DivW();
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
