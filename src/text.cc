#include "text.h"

#include "gfx.h"
#include "gfxview.h"

#include "vertex.h"
#include "texture.h"

#include <unordered_map>

struct TextFont {
  const Texture *texture;
  Point size;

  std::string name;

  TextFont() :
    texture(0),
    size(0,0)
  {}

  TextFont(const std::string &name) :
    texture(Texture::Get("gui/font."+name)),
    size( texture->size.x / 32, texture->size.y / 8 ),
    name(name)
  {
  }
};

static std::unordered_map<std::string, TextFont> fonts;

static const TextFont &loadTextFont(const std::string &name) {
  if (fonts.find(name) != fonts.end()) return fonts[name];
  fonts[name] = TextFont(name);
  return fonts[name];
}

static const char *
utf8ToWide(
  const char *s,
  wchar_t *out
) {
  if (!s) return nullptr;

  if (!*s) {
    if (out) *out = 0;
    return nullptr;
  }

  if (*s > 0) {
    if (out) *out = *s;
    return s + 1;
  }

  wchar_t c = 0;
  const char *p = s;

  if ((*p & 0xE0) == 0xC0) {
    c = *p & 0x1F;
    p++;
    if ((*p & 0xC0) != 0x80)
      goto decode_error;

    c = (c << 6) | (*p & 0x3F);
    p++;
  } else if ((*p & 0xF0) == 0xE0) {
    c = *p & 0x0F;
    p++;
    if ((*p & 0xC0) != 0x80)
      goto decode_error;
    c = (c << 6) | (*p & 0x3F);
    p++;
    if ((*p & 0xC0) != 0x80)
      goto decode_error;
    c = (c << 6) | (*p & 0x3F);
    p++;
  } else if ((*p & 0xF8) == 0xF0) {
    c = *p & 0x07;
    p++;
    if ((*p & 0xC0) != 0x80)
      goto decode_error;
    c = (c << 6) | (*p & 0x3F);
    p++;
    if ((*p & 0xC0) != 0x80)
      goto decode_error;
    c = (c << 6) | (*p & 0x3F);
    p++;
    if ((*p & 0xC0) != 0x80)
      goto decode_error;
    c = (c << 6) | (*p & 0x3F);
    p++;
  } else {
    goto decode_error;
  }

  if (out) *out = c;
  return p;

decode_error:

  if (out) *out = 0xFFFD;		/* "replace" character */
  while (p && ((*p & 0xC0) == 0x80)) {
    p++;
  }
  return p;
}

RenderString::RenderString(const std::string &text, const std::string &fontName) :
  font(loadTextFont(fontName)),
  mbString(""),
  text(L""),
  wrappedText(L""),
  dirty(true),
  vertices(),
  size(0,0)
{
  this->SetText(text);
}

RenderString& RenderString::operator =(const std::string &text) {
  this->SetText(text);
  this->dirty = true;
  return *this;
}

RenderString::~RenderString() {
}

void RenderString::Draw(Gfx &gfx, float x, float y, int align) {
  if (dirty) {
    this->DrawString();
  }

  if (align & (int)Align::HorizRight) {
    x = x - size.x;
  } else if (align & (int)Align::HorizCenter) {
    x = x - size.x / 2;
  }

  if (align & (int)Align::VertBottom) {
    y = y - size.y;
  } else if (align & (int)Align::VertMiddle) {
    y = y - size.y / 2;
  }

  gfx.SetTextureFrame(this->font.texture);
  gfx.GetView().Push();
  gfx.GetView().Translate(Vector3(x,y,0));
  gfx.DrawQuads(vertices);
  gfx.GetView().Pop();
}

void RenderString::Draw(Gfx &gfx, const Point &pos, int align) {
  Draw(gfx, pos.x, pos.y, align);
}

void RenderString::DrawChar(float x, float y, wchar_t c, const IColor &color) {
  float u =   (c%32)/32.0;
  float v = 1-(c/32)/ 8.0;
  Point size = font.size;

  for (int xx = -1; xx<2; xx++) for (int yy = -1; yy<2; yy++) {
    this->vertices.Add(Vertex(Vector3(x+size.x+xx,      0+yy+y, 0.1), IColor(), u+1.0/32.0,v));
    this->vertices.Add(Vertex(Vector3(x       +xx,      0+yy+y, 0.1), IColor(), u,v));
    this->vertices.Add(Vertex(Vector3(x       +xx, size.y+yy+y, 0.1), IColor(), u,v-1.0/8.0));
    this->vertices.Add(Vertex(Vector3(x+size.x+xx, size.y+yy+y, 0.1), IColor(), u+1.0/32.0,v-1.0/8.0));
  }

  this->vertices.Add(Vertex(Vector3(x+size.x,      0+y, 0), color, u+1.0/32.0,v));
  this->vertices.Add(Vertex(Vector3(x       ,      0+y, 0), color, u,v));
  this->vertices.Add(Vertex(Vector3(x       , size.y+y, 0), color, u,v-1.0/8.0));
  this->vertices.Add(Vertex(Vector3(x+size.x, size.y+y, 0), color, u+1.0/32.0,v-1.0/8.0));
}

void RenderString::DrawString() {
  float x = 0;
  float y = 0;
  const Point &size = font.size;
  IColor color(255,255,255);

  this->vertices.Clear();
  this->dirty = false;

  if (this->text == L"") return;

  const wchar_t *p = this->wrappedText.c_str();
  float maxX = 0;
  float maxY = size.y;

  while (*p) {
    wchar_t wchar = *p++;

    switch(wchar) {
      // handle newlines
      case L'\n': {
        x = 0;
        y += size.y;
        maxY += size.y;
        continue;
      }

      case 0xFE00: color = IColor(  0,   0,   0); continue;
      case 0xFE01: color = IColor(  0,   0, 128); continue;
      case 0xFE02: color = IColor(  0, 128,   0); continue;
      case 0xFE03: color = IColor(  0, 128, 128); continue;
      case 0xFE04: color = IColor(128,   0,   0); continue;
      case 0xFE05: color = IColor(128,   0, 128); continue;
      case 0xFE06: color = IColor(128, 128,   0); continue;
      case 0xFE07: color = IColor(192, 192, 192); continue;
      case 0xFE08: color = IColor(128, 128, 128); continue;
      case 0xFE09: color = IColor(  0,   0, 255); continue;
      case 0xFE0A: color = IColor(  0, 255,   0); continue;
      case 0xFE0B: color = IColor(  0, 255, 255); continue;
      case 0xFE0C: color = IColor(255,   0,   0); continue;
      case 0xFE0D: color = IColor(255,   0, 255); continue;
      case 0xFE0E: color = IColor(255, 255,   0); continue;
      case 0xFE0F: color = IColor(255, 255, 255); continue;

      default: break;
    }

    this->DrawChar(x, y, wchar, color);

    x += size.x;
    if (x > maxX) maxX = x;
  }

  this->size = Point(maxX, maxY);
}

const Point &
RenderString::GetSize() {
  if (dirty) {
    this->DrawString();
  }
  return size;
}

const std::string &RenderString::GetFontName() const {
  return this->font.name;
}

void
RenderString::WrapWords(size_t width) {
  this->dirty = true;

  int lastSpace = -1;
  size_t p = 0;
  size_t x = 0;

  this->wrappedText = this->text;

  while(p < this->wrappedText.size()) {
    size_t w = this->font.size.x;
    wchar_t c = this->wrappedText[p];

    if (c == L'\n') {
      p++;
      lastSpace = -1;
      x = 0;
      continue;
    }

    if (c >= 0xFE00 && c <= 0xFE0F) {
      p++;
      continue;
    }

    if (c == L' ') {
      lastSpace = p;
    }

    if (x+w > width && lastSpace != -1) {
      this->wrappedText[lastSpace] = '\n';
      lastSpace = -1;
      x = 0;
      w = 0;
    }

    x += w;
    p++;
  }
}

void
RenderString::SetText(const std::string &text) {
  const char *p = text.c_str();
  this->mbString = text;
  this->text = L"";

  while (*p) {
    // convert utf8 to wchar_t
    wchar_t wchar;
    p = utf8ToWide(p, &wchar);
    if (!wchar || !p) break;
    this->text += wchar;
  }
  this->wrappedText = this->text;

  this->dirty = true;
}
