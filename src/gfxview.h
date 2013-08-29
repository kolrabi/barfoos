#ifndef BARFOOS_GFXVIEW_H
#define BARFOOS_GFXVIEW_H

#include "common.h"
#include "vector3.h"

class GfxView final {
public:

  void Look(const Vector3 &pos, const Vector3 &forward, float fovY = 60.0, const Vector3 &up = Vector3(0,1,0));
  void GUI();
  
  void Push();
  void Pop();
  void Translate(const Vector3 &p);
  void Scale(const Vector3 &p);
  void Rotate(float angle, const Vector3 &p);

  Vector3 WorldToScreen(const Vector3 &p) const;
  Vector3 WorldToScreen(const Vector3 &p, bool &visible) const;
  bool IsPointVisible(const Vector3 &p) const;
  bool IsAABBVisible(const AABB &p) const;
 
  const Vector3 &GetRight() const { return right; }

  void Billboard(bool flip = false, bool vertical = false);
  
private:

  Gfx &gfx;
  
  std::vector<Matrix4> projStack;
  std::vector<Matrix4> viewStack;
  std::vector<Matrix4> modelViewStack;
  std::vector<Matrix4> textureStack;

  Vector3 right;

  friend class Gfx;
  
  GfxView(Gfx &gfx) : 
    gfx(gfx),
    projStack(1),
    viewStack(1),
    modelViewStack(1),
    textureStack(1)
  {}
  
  void SetUniforms(const std::shared_ptr<Shader> &shader) const;
};

#endif

