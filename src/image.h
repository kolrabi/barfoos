#ifndef BARFOOS_IMAGE_H
#define BARFOOS_IMAGE_H

#include "common.h"
#include "2d.h"
#include "vector3.h"

/** An image. */
class Image {
public:
  Image();
  Image(const Point &size, uint8_t *data, bool alpha = false);
  Image(Image &) = delete;
  Image(Image &&);
  ~Image();
  
  void Save(const std::string &name);
  
  const Point &GetSize() const { return size; }
  const void *GetData() const { return rgba; }
  bool HasAlpha() const { return hasAlpha; }

  static Image Noise(const Point &size, const Vector3 &scale = Vector3(1,1,1), const Vector3 &offset = Vector3());
  static Image Load(const std::string &name);
  
private:

  bool hasAlpha;
  uint8_t *rgba;
  
  Point size;
};

#endif

