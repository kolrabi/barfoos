#ifndef BARFOOS_TEXT_H
#define BARFOOS_TEXT_H

#include "common.h"

struct Vertex;
struct IColor;
class Texture;
class Gfx;

struct Font {
  const Texture *texture;
  Point size;
};

class RenderString {
public:

  RenderString(const std::string &text, const std::string &font = "default");
  RenderString &operator =(const std::string &text);

  void Draw(Gfx &gfx, float x, float y);
  
  const Font &GetFont() const { return font; }

private:

  const Font &font;
  std::string text;
  bool dirty;
  std::vector<Vertex> vertices;
  
  void DrawChar(float x, float y, wchar_t c, const IColor &color);
  void DrawString();
};

#endif

