#include "text.h"
#include "gfx.h"
#include "vertex.h"

static Point size(8,16);

RenderString::RenderString(const std::string &text)
:text(text), dirty(true) {
  this->texture = loadTexture("gui/fontbold");
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
  
  gfx.SetTextureFrame(this->texture);
  gfx.ViewPush();
  gfx.ViewTranslate(Vector3(x,y,0));
  gfx.DrawQuads(vertices);
  gfx.ViewPop();
}

void RenderString::DrawChar(float x, float y, wchar_t c) {
  float u =   (c%32)/32.0;
  float v = 1-(c/32)/ 8.0;

  this->vertices.push_back(Vertex(Vector3(x+size.x+1, size.y+1+y, 0.1), IColor(), u+1.0/32.0,v-1.0/8.0));
  this->vertices.push_back(Vertex(Vector3(x       +1, size.y+1+y, 0.1), IColor(), u,v-1.0/8.0));
  this->vertices.push_back(Vertex(Vector3(x       +1,      0+1+y, 0.1), IColor(), u,v));
  this->vertices.push_back(Vertex(Vector3(x+size.x+1,      0+1+y, 0.1), IColor(), u+1.0/32.0,v));

  this->vertices.push_back(Vertex(Vector3(x+size.x,      0+y, 0), IColor(255, 255, 255), u+1.0/32.0,v));
  this->vertices.push_back(Vertex(Vector3(x       ,      0+y, 0), IColor(255, 255, 255), u,v));
  this->vertices.push_back(Vertex(Vector3(x       , size.y+y, 0), IColor(255, 255, 255), u,v-1.0/8.0));
  this->vertices.push_back(Vertex(Vector3(x+size.x, size.y+y, 0), IColor(255, 255, 255), u+1.0/32.0,v-1.0/8.0));
}

void RenderString::DrawString() {
  float x = 0;
  float y = 0;

  this->vertices = std::vector<Vertex>(); 
  this->dirty = false;
  
  const char *p = this->text.c_str();
  size_t max = this->text.length();
  int length = ::mbtowc(0,0,0);

  while (*p) {
    // handle newlines
    if (*p == '\n') {
      x = 0;
      y += size.y;
      p ++;
      continue;
    }
    
    // convert utf8 to wchar_t
    wchar_t wchar;
    length = ::mbtowc(&wchar, p, max);
    if (length < 1) break;
    
    this->DrawChar(x, y, wchar);
    
    max -= length;
    p += length;
    x += size.x;
  }
}
