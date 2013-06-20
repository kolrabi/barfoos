#include "common.h"

#include <GL/glfw.h>

extern int screenWidth, screenHeight;

void drawUnitCube() {
  glBegin(GL_QUADS);
    
  glVertex3f(-1, 1,-1);
  glVertex3f( 1, 1,-1);
  glVertex3f( 1,-1,-1);
  glVertex3f(-1,-1,-1);

  glVertex3f(-1, 1, 1);
  glVertex3f( 1, 1, 1);
  glVertex3f( 1,-1, 1);
  glVertex3f(-1,-1, 1);

  glVertex3f(-1, 1, 1);
  glVertex3f( 1, 1, 1);
  glVertex3f( 1, 1, -1);
  glVertex3f(-1, 1, -1);

  glVertex3f(-1,-1, 1);
  glVertex3f( 1,-1, 1);
  glVertex3f( 1,-1, -1);
  glVertex3f(-1,-1, -1);

  glVertex3f( 1,-1, 1);
  glVertex3f( 1, 1, 1);
  glVertex3f( 1, 1,-1);
  glVertex3f( 1,-1,-1);

  glVertex3f(-1,-1, 1);
  glVertex3f(-1, 1, 1);
  glVertex3f(-1, 1,-1);
  glVertex3f(-1,-1,-1);
  glEnd();
}    

void drawAABB(const AABB &aabb) {
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glTranslatef(aabb.center.x, aabb.center.y, aabb.center.z);
  glScalef(aabb.extents.x, aabb.extents.y, aabb.extents.z);
  drawUnitCube();
  glPopMatrix();
}

void drawBillboard(const Vector3 &pos, float w, float h, unsigned int tex, float u, float uw, float ofsX, float ofsY) {
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  
  glTranslatef(pos.x, pos.y, pos.z);
  
  float m[16];
  glGetFloatv(GL_MODELVIEW_MATRIX, m);
  
  //unsigned int tex = texture;
  //bool lookingDown = m[6] >  0.8;
  //bool lookingUp   = m[6] < -0.8;
  
  //if (lookingUp   && belowTexture != 0) tex = belowTexture;
  //if (lookingDown && aboveTexture != 0) tex = aboveTexture;
  
  m[0] = 1; m[1] = 0; m[2]  = 0;
  //m[4] = 0; m[5] = 1; m[6]  = 0;
  m[8] = 0; m[9] = 0; m[10] = 1;
  
  glLoadMatrixf(m);
  glTranslatef(ofsX, ofsY, 0);
 
  glBindTexture(GL_TEXTURE_2D, tex);

  glBegin(GL_QUADS);
  glTexCoord2f(u,   1); glVertex2f(-w, h);
  glTexCoord2f(u,   0); glVertex2f(-w,-h);
  glTexCoord2f(u+uw,0); glVertex2f( w,-h);
  glTexCoord2f(u+uw,1); glVertex2f( w, h);
  glEnd();

  glPopMatrix();
}

void viewGUI() {
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  
  glScalef(2.0/screenWidth, -2.0/screenHeight, 1);
  glTranslatef(-screenWidth/2,-screenHeight/2, 0);
}

void drawIcon(float x, float y, float w, float h, unsigned int tex, float u, float uw) {
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  
  glTranslatef(x, y, 0);
  
  glBindTexture(GL_TEXTURE_2D, tex);

  glBegin(GL_QUADS);
  glTexCoord2f(u,   0); glVertex2f(-w, h);
  glTexCoord2f(u,   1); glVertex2f(-w,-h);
  glTexCoord2f(u+uw,1); glVertex2f( w,-h);
  glTexCoord2f(u+uw,0); glVertex2f( w, h);
  glEnd();

  glPopMatrix();
}
