#ifndef BARFOOS_TEXTURE_H
#define BARFOOS_TEXTURE_H

#include "2d.h"
#include "vector3.h"

class Image;

/** A texture. */
struct Texture {
  Texture();
  Texture(const std::string &name);
  Texture(Texture &&);
  ~Texture();

  std::string name;

  /** OpenGL Handle of the texture. */
  unsigned int handle;

  /** Size of the texture. */
  Point size;

  void Reload();
  void SetImage(const Image &image);

  static void UpdateTextures();
  static const Texture *Get(const std::string &name);
  static const Texture *Create(const std::string &name, const Image &image);
};

#endif

