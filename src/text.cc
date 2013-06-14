#include "text.h"
#include "util.h"

#include <GL/glfw.h>

extern int screenWidth;
extern int screenHeight;

static GLuint tex = 0;
static float scaleX = 8, scaleY = 16;

static void drawChar(float x, uint8_t c, std::vector<Vertex> &verts) {
  float u =   (c%32)/32.0;
  float v = 1-(c/32)/ 8.0;

  verts.push_back(Vertex(Vector3(x+1+0.1, 0+0.1, 0), IColor(), u+1.0/32.0,v));
  verts.push_back(Vertex(Vector3(x  +0.1, 0+0.1, 0), IColor(), u,v));
  verts.push_back(Vertex(Vector3(x  +0.1, 1+0.1, 0), IColor(), u,v-1.0/8.0));
  verts.push_back(Vertex(Vector3(x+1+0.1, 1+0.1, 0), IColor(), u+1.0/32.0,v-1.0/8.0));

  verts.push_back(Vertex(Vector3(x+1, 0, 0), IColor(255, 255, 255), u+1.0/32.0,v));
  verts.push_back(Vertex(Vector3(x  , 0, 0), IColor(255, 255, 255), u,v));
  verts.push_back(Vertex(Vector3(x  , 1, 0), IColor(255, 255, 255), u,v-1.0/8.0));
  verts.push_back(Vertex(Vector3(x+1, 1, 0), IColor(255, 255, 255), u+1.0/32.0,v-1.0/8.0));
}

static void drawString(const std::string &text, std::vector<Vertex> &verts) {
  const char *p = text.c_str();
  float x = 0;

  while (*p) {
    drawChar(x, *p, verts);
    p++;
    x++;
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
  if (tex == 0) tex = loadTexture("gui/font");

  if (dirty) { 
    vertices = std::vector<Vertex>(); 
    drawString(str, vertices);
    dirty = false;
  }
  
  if (screenWidth <= 320) scaleX = 8;
  else if (screenWidth <= 640) scaleX = 16;
  else scaleX = 32;
  scaleY = scaleX*2;
  
  glBindTexture(GL_TEXTURE_2D, tex);

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
  
  glScalef(2.0/screenWidth, -2.0/screenHeight, 1);
  glTranslatef(-screenWidth/2,-screenHeight/2, 0);
  glScalef(scaleX, scaleY, 1);

  glEnableClientState(GL_VERTEX_ARRAY);
  //glEnableClientState(GL_COLOR_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  
  glBindTexture(GL_TEXTURE_2D, tex);
  glInterleavedArrays(GL_T2F_C3F_V3F, sizeof(Vertex), &vertices[0]);

  glColor3ub(255, 255, 0);  
  glTranslatef(x+0.5, y+0.5, 0);
  glDrawArrays(GL_QUADS,0, vertices.size());

  glDisableClientState(GL_TEXTURE_COORD_ARRAY);
  //glDisableClientState(GL_COLOR_ARRAY);
  glDisableClientState(GL_VERTEX_ARRAY);

  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
}
