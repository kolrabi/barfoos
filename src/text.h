#ifndef BARFOOS_TEXT_H
#define BARFOOS_TEXT_H

#include "common.h"

struct Vertex;

class Texture;
class Gfx;

class RenderString {
public:

  RenderString(const std::string &text);
  RenderString &operator =(const std::string &text);

  void Draw(Gfx &gfx, float x, float y);

private:

  const Texture *texture;
  std::string text;
  bool dirty;
  std::vector<Vertex> vertices;
  
  void DrawChar(float x, float y, wchar_t c);
  void DrawString();
};

#endif

