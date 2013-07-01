#include "text.h"
#include "util.h"

#include <GL/glfw.h>
#include "gfx.h"

static const Texture *tex = nullptr;
static Point size(8,16);

static void drawChar(float x, float y, wchar_t c, std::vector<Vertex> &verts) {
  float u =   (c%32)/32.0;
  float v = 1-(c/32)/ 8.0;

  verts.push_back(Vertex(Vector3(x+size.x+1, size.y+1+y, 0.1), IColor(), u+1.0/32.0,v-1.0/8.0));
  verts.push_back(Vertex(Vector3(x       +1, size.y+1+y, 0.1), IColor(), u,v-1.0/8.0));
  verts.push_back(Vertex(Vector3(x       +1,      0+1+y, 0.1), IColor(), u,v));
  verts.push_back(Vertex(Vector3(x+size.x+1,      0+1+y, 0.1), IColor(), u+1.0/32.0,v));

  verts.push_back(Vertex(Vector3(x+size.x,      0+y, 0), IColor(255, 255, 255), u+1.0/32.0,v));
  verts.push_back(Vertex(Vector3(x       ,      0+y, 0), IColor(255, 255, 255), u,v));
  verts.push_back(Vertex(Vector3(x       , size.y+y, 0), IColor(255, 255, 255), u,v-1.0/8.0));
  verts.push_back(Vertex(Vector3(x+size.x, size.y+y, 0), IColor(255, 255, 255), u+1.0/32.0,v-1.0/8.0));
}

static void drawString(const std::string &text, std::vector<Vertex> &verts) {
  float x = 0;
  float y = 0;

  const char *p = text.c_str();
  size_t max = text.length();
  int l = mbtowc(0,0,0);

  while (*p) {
    if (*p == '\n') {
      x = 0;
      y += size.y;
      p ++;
      continue;
    }
    wchar_t wc;
    l = mbtowc(&wc, p, max);
    if (l < 1) break;
    max -= l;
    p += l;
    drawChar(x, y, wc, verts);
    x+=size.x;
  }
}

RenderString::RenderString(const std::string &str)
:str(str), dirty(true) {
}

RenderString& RenderString::operator =(const std::string &str) {
  this->str = str;
  this->dirty = true;
  return *this;
}

void RenderString::Draw(float x, float y) {
  if (!tex) tex = loadTexture("gui/fontbold");

  if (dirty) { 
    vertices = std::vector<Vertex>(); 
    drawString(str, vertices);
    dirty = false;
  }
  
  Gfx::Instance->SetTextureFrame(tex);
  
  glPushMatrix();
  glTranslatef(x,y,0);
  glDisable(GL_DEPTH_TEST);
  
  Gfx::Instance->DrawQuads(vertices);
  
  glPopMatrix();
}
