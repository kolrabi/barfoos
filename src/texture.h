#ifndef BARFOOS_TEXTURE_H
#define BARFOOS_TEXTURE_H

#include "common.h"

/** A texture. */
struct Texture {
  Texture();
  Texture(Texture &&);
  ~Texture();
  
  /** OpenGL Handle of the texture. */
  unsigned int handle;
  
  /** Size of the texture. */
  Point size;
};

const Texture *noiseTexture(const Point &size, const Vector3 &scale = Vector3(1,1,1), const Vector3 &offset = Vector3());
const Texture *loadTexture(const std::string &name, const Texture * tex = nullptr);
void updateTextures();

#endif

