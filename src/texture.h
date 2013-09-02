#ifndef BARFOOS_TEXTURE_H
#define BARFOOS_TEXTURE_H

#include "2d.h"
#include "vector3.h"

class Image;

/** A texture. */
struct Texture {
  Texture();
  Texture(Texture &&);
  ~Texture();
  
  std::string name;
  
  /** OpenGL Handle of the texture. */
  unsigned int handle;
  
  /** Size of the texture. */
  Point size;
};

// TODO: static members of Texture
const Texture *noiseTexture(const Point &size, const Vector3 &scale = Vector3(1,1,1), const Vector3 &offset = Vector3());
const Texture *loadTexture(const std::string &name, const Texture * tex = nullptr);
const Texture *updateTexture(const std::string &name, const Image &image);
void updateTextures();

#endif

