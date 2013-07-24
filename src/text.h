#ifndef BARFOOS_TEXT_H
#define BARFOOS_TEXT_H

#include "common.h"
#include "2d.h"
#include "vertex.h"

struct Font;

enum class Align {
  HorizLeft = 0,
  HorizRight = 1,
  HorizCenter = 2,
  
  VertTop = 0,
  VertMiddle = 4,
  VertBottom = 8
};

static inline Align operator|(Align a, Align b) { return Align(int(a) | int(b)); }

class RenderString {
public:

  RenderString(const std::string &text, const std::string &font = "default");
  RenderString &operator =(const std::string &text);

  void Draw(Gfx &gfx, float x, float y, int align = 0);
  void Draw(Gfx &gfx, const Point &pos, int align = 0);
  
  void WrapWords(size_t width);
  
  const Font & GetFont()  const { return font; }
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

