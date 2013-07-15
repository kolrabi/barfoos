#ifndef BARFOOS_TEXT_H
#define BARFOOS_TEXT_H

#include "common.h"
#include "2d.h"

struct Font;

enum class Align {
  Left = 0,
  Right = 1,
  Center = 2,
  
  Top = 0,
  Middle = 4,
  Bottom = 8
};

class RenderString {
public:

  RenderString(const std::string &text, const std::string &font = "default");
  RenderString &operator =(const std::string &text);

  void Draw(Gfx &gfx, float x, float y, int align = 0);
  
  void WrapWords(size_t width);
  const Font &GetFont()  const { return font; }
  const Point &GetSize();

private:

  const Font &font;
  std::wstring text;
  std::wstring wrappedText;
  size_t dirty;
  std::vector<Vertex> vertices;
  Point size;
  
  void DrawChar(float x, float y, wchar_t c, const IColor &color);
  void DrawString();
  
  void SetText(const std::string &text);
};

#endif

