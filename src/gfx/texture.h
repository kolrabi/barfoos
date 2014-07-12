#ifndef BARFOOS_TEXTURE_H
#define BARFOOS_TEXTURE_H

#include "math/2d.h"

class Image;
class Vector2;

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

  void GetFrameUV(size_t frame, size_t totalFrames, Vector2 &uv1, Vector2 &uv2) const;

  static void UpdateTextures();
  static const Texture *Get(const std::string &name);
  static const Texture *Create(const std::string &name, const Image &image);
};

#endif

