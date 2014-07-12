#ifndef BARFOOS_GFXVIEW_H
#define BARFOOS_GFXVIEW_H

#include "common.h"
#include "math/vector3.h"
#include "math/matrix4.h"

class GfxView final {
public:

  void            Look            (const Vector3 &pos, const Vector3 &forward, float fovY = 60.0, const Vector3 &up = Vector3(0,1,0));
  void            GUI             ();

  void            Push            ();
  void            Pop             ();
  void            Translate       (const Vector3 &p);
  void            Scale           (const Vector3 &p);
  void            Rotate          (float angle, const Vector3 &p);

  Vector3         WorldToScreen   (const Vector3 &p)                          const;
  Vector3         WorldToScreen   (const Vector3 &p, bool &visible)           const;
  bool            IsPointVisible  (const Vector3 &p)                          const;
  bool            IsAABBVisible   (const AABB &p)                             const;

  const Vector3 & GetPos          ()                                          const { return this->pos; }
  const Vector3 & GetForward      ()                                          const { return this->forward; }
  const Vector3 & GetUp           ()                                          const { return this->up; }
  const Vector3 & GetRight        ()                                          const { return this->right; }

  void            Billboard       (bool flip = false, bool vertical = false);
  void            AlignY          (const Vector3 &forward);

private:

  friend class Gfx;

                  GfxView         (Gfx &gfx);

  Gfx &           gfx;

  Matrix4         projectionMatrix;
  Matrix4         viewMatrix;
  std::vector<Matrix4> modelMatrixStack;
  Matrix4         textureMatrix;

  Vector3         pos, forward, up, right;

  void SetUniforms(const std::shared_ptr<Shader> &shader) const;
};

#endif

