#include "text.h"
#include "gfx.h"
#include "vertex.h"
#include "texture.h"

static std::map<std::string, Font> fonts;

static const Font &loadFont(const std::string &name) {
  if (fonts.find(name) != fonts.end()) return fonts[name];
  Font font;
  font.texture = loadTexture("gui/font."+name);
  font.size = Point( font.texture->size.x / 32, font.texture->size.y / 8);
  fonts[name] = font;
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


RenderString::RenderString(const std::string &text, const std::string &font)
: font(loadFont(font)), text(text), dirty(true) {
}

RenderString& RenderString::operator =(const std::string &text) {
  this->text = text;
  this->dirty = true;
  return *this;
}

void RenderString::Draw(Gfx &gfx, float x, float y) {
  if (dirty) { 
    this->DrawString();
  }
  
  gfx.SetTextureFrame(this->font.texture);
  gfx.GetView().Push();
  gfx.GetView().Translate(Vector3(x,y,0));
  gfx.DrawQuads(vertices);
  gfx.GetView().Pop();
}

void RenderString::DrawChar(float x, float y, wchar_t c, const IColor &color) {
  float u =   (c%32)/32.0;
  float v = 1-(c/32)/ 8.0;
  Point size = font.size;

  this->vertices.push_back(Vertex(Vector3(x+size.x+1, size.y+1+y, 0.1), IColor(), u+1.0/32.0,v-1.0/8.0));
  this->vertices.push_back(Vertex(Vector3(x       +1, size.y+1+y, 0.1), IColor(), u,v-1.0/8.0));
  this->vertices.push_back(Vertex(Vector3(x       +1,      0+1+y, 0.1), IColor(), u,v));
  this->vertices.push_back(Vertex(Vector3(x+size.x+1,      0+1+y, 0.1), IColor(), u+1.0/32.0,v));

  this->vertices.push_back(Vertex(Vector3(x+size.x,      0+y, 0), color, u+1.0/32.0,v));
  this->vertices.push_back(Vertex(Vector3(x       ,      0+y, 0), color, u,v));
  this->vertices.push_back(Vertex(Vector3(x       , size.y+y, 0), color, u,v-1.0/8.0));
  this->vertices.push_back(Vertex(Vector3(x+size.x, size.y+y, 0), color, u+1.0/32.0,v-1.0/8.0));
}

void RenderString::DrawString() {
  float x = 0;
  float y = 0;
  const Point &size = font.size;
  IColor color(255,255,255);

  this->vertices = std::vector<Vertex>(); 
  this->dirty = false;

  if (this->text == "") return;
  
  const char *p = this->text.c_str();
  
  while (*p) {
    // convert utf8 to wchar_t
    wchar_t wchar;
    p = utf8ToWide(p, &wchar);
    if (!p) break;

    switch(wchar) {
    // handle newlines
      case '\n': {
        x = 0;
        y += size.y;
        p ++;
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
  }
}
