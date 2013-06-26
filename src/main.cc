#include "GLee.h"
#include <GL/glfw.h>

#include "common.h"
#include "util.h"
#include "world.h"
#include "player.h"
#include "game.h"
#include <png.h>
#include <zlib.h>

#include <cctype>
#include <cstring>

#include "simplex.h"

size_t level = 0;

size_t guiActive = false;
bool mouseGrab = false;
int screenWidth = 640;
int screenHeight = 400;
float mouseDX = 0;
float mouseDY = 0;

int mouseX = 0, mouseY = 0;

float Wave(float x, float z, float t, float a, float o) {
  return o + a * cos( ((x+z+t)*0.4 )*0.1) * cos( ((x-z  )*0.6+t*0.3) * 0.5) * 0.5;
}

int virtualScreenWidth, virtualScreenHeight;

void updateScreenSize() {  
  if (screenWidth <= 640) {
    virtualScreenWidth = screenWidth;
    virtualScreenHeight = screenHeight;
  } else {
    virtualScreenWidth = screenWidth/2;
    virtualScreenHeight = screenHeight/2;
  }
}

void mouseMove(int x, int y) {
  if (guiActive) {
    mouseDX = mouseDY = 0;
    mouseX = (x / (float)screenWidth)*virtualScreenWidth;
    mouseY = (y / (float)screenHeight)*virtualScreenHeight;
    Game::Instance->OnMouseMove(Point(mouseX,mouseY));
  } else if (mouseGrab) {
    mouseDX = (x-screenWidth/2)*0.005;
    mouseDY = (y-screenHeight/2)*0.005;
    glfwSetMousePos(screenWidth/2, screenHeight/2);
  }
}

void mouseClick(int button, int down) {
  if (Game::Instance) {
    if (!mouseGrab && down && button == GLFW_MOUSE_BUTTON_LEFT) {
      if (!guiActive) {
        glfwSetMousePos(screenWidth/2, screenHeight/2);
        // glfwDisable(GLFW_MOUSE_CURSOR);
      }
      mouseGrab = true;
    } else {
      Game::Instance->OnMouseClick(Point(mouseX,mouseY), button, down);
    }
  }
}

void keyEvent(int key, int action) {
  if (key == GLFW_KEY_F12 && action == GLFW_PRESS) {
    new Game("seed", ++level);
  }
}

void resize(int x, int y) {
  glViewport(0,0, x,y);
  screenWidth = x;
  screenHeight = y;
  
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(60.0f, (float)screenWidth/(float)screenHeight, 0.0015f, 64.0f);

  glMatrixMode(GL_MODELVIEW);
  updateScreenSize();
}

static std::string credits() {
  std::string str;

  str += " GLFW - An OpenGL framework Version 2.7.8";
  str += "\n"
         "   Copyright (c) 2002-2006 Marcus Geelnard\n"
         "   Copyright (c) 2006-2010 Camilla Berglund\n";
  str += "\n";
  
  str += " GLee (OpenGL Easy Extension library) Version 5.2\n"
         "   Copyright (c)2006  Ben Woodhouse  All rights reserved.\n";
  str += "\n";

  str += " zlib version ";
  str += ZLIB_VERSION;
  str += "\n"
         "   Copyright (C) 1995-2012 Jean-loup Gailly and Mark Adler\n";
         
  str += png_get_copyright(nullptr);  
  return str;
}

int saveImage(const std::string &fileName, size_t w, size_t h, const uint8_t *rgb);

int main() {
  std::cerr << credits() << std::endl;
  std::setlocale(LC_ALL, "en_US.utf8");
  /*
  size_t w = 512, h = 512;
  uint8_t *data = new uint8_t[w*h*3];
  
  for (size_t x = 0; x<w; x++) {
    for (size_t y = 0; y<h; y++) {
      Vector3 p(x,y,0);
      float f = (simplexNoise(p*0.1) * 0.5 + 0.5)*255;
      data[(x+y*w)*3 + 0] = f;
      data[(x+y*w)*3 + 1] = f;
      data[(x+y*w)*3 + 2] = f;
    }
  }
  saveImage("test.png", w,h,data);
  
  for (size_t x = 0; x<w; x++) {
    for (size_t y = 0; y<h; y++) {
      Vector3 p(x,y,0);
      float f = (simplexNoise(p*0.1) * simplexNoise(p*0.05) * 0.5 + 0.5)*255;
      data[(x+y*w)*3 + 0] = f;
      data[(x+y*w)*3 + 1] = f;
      data[(x+y*w)*3 + 2] = f;
    }
  }
  saveImage("test-2o-mul.png", w,h,data);

  for (size_t x = 0; x<w; x++) {
    for (size_t y = 0; y<h; y++) {
      Vector3 p(x,y,0);
      float f = (simplexNoise(p*0.025) * simplexNoise(p*0.2) * simplexNoise(p*0.1) * simplexNoise(p*0.05) * 10 + 0.5)*255;
      if (f < 0) f = 0; if (f > 255) f = 255;
      data[(x+y*w)*3 + 0] = f;
      data[(x+y*w)*3 + 1] = f;
      data[(x+y*w)*3 + 2] = f;
    }
  }
  saveImage("test-4o-mul.png", w,h,data);

  for (size_t x = 0; x<w; x++) {
    for (size_t y = 0; y<h; y++) {
      Vector3 p(x,y,0);
      float f = std::abs(simplexNoise(p*0.025) * simplexNoise(p*0.2) * simplexNoise(p*0.1) * simplexNoise(p*0.05) * 5)*255;
      if (f < 0) f = 0; if (f > 255) f = 255;
      data[(x+y*w)*3 + 0] = f;
      data[(x+y*w)*3 + 1] = f;
      data[(x+y*w)*3 + 2] = f;
    }
  }
  saveImage("test-4o-mul-abs.png", w,h,data);

  for (size_t x = 0; x<w; x++) {
    for (size_t y = 0; y<h; y++) {
      Vector3 p(x,y,0);
      float f = std::abs(simplexNoise(p*0.025)) * std::abs(simplexNoise(p*0.2)) * std::abs(simplexNoise(p*0.1)) * std::abs(simplexNoise(p*0.05)) * 5 *255;
      if (f < 0) f = 0; if (f > 255) f = 255;
      data[(x+y*w)*3 + 0] = f;
      data[(x+y*w)*3 + 1] = f;
      data[(x+y*w)*3 + 2] = f;
    }
  }
  saveImage("test-4o-abs-mul.png", w,h,data);
  
  for (size_t x = 0; x<w; x++) {
    for (size_t y = 0; y<h; y++) {
      Vector3 p(x,y,0);
      float f = ((simplexNoise(p*0.1) + simplexNoise(p*0.05)) * 0.25 + 0.5)*255;
      data[(x+y*w)*3 + 0] = f;
      data[(x+y*w)*3 + 1] = f;
      data[(x+y*w)*3 + 2] = f;
    }
  }
  saveImage("test-2o-add.png", w,h,data);

  for (size_t x = 0; x<w; x++) {
    for (size_t y = 0; y<h; y++) {
      Vector3 p(x,y,0);
      float f = (simplexNoise(p*0.025) + simplexNoise(p*0.2) + (simplexNoise(p*0.1) + simplexNoise(p*0.05)) / 8.0 + 0.5)*255;
      data[(x+y*w)*3 + 0] = f;
      data[(x+y*w)*3 + 1] = f;
      data[(x+y*w)*3 + 2] = f;
    }
  }
  saveImage("test-4o-add.png", w,h,data);  
  
  for (size_t x = 0; x<w; x++) {
    for (size_t y = 0; y<h; y++) {
      Vector3 p(x,y,0);
      p.x += simplexNoise(p*0.1 + Vector3(0,0, 1))*2;
      p.y += simplexNoise(p*0.1 + Vector3(0,0,-1))*2;
      
      float f = (simplexNoise(p*0.1) * 0.5 + 0.5)*255;
      data[(x+y*w)*3 + 0] = f;
      data[(x+y*w)*3 + 1] = f;
      data[(x+y*w)*3 + 2] = f;
    }
  }
  saveImage("test-ofs-add.png", w,h,data);
  
  for (size_t x = 0; x<w; x++) {
    for (size_t y = 0; y<h; y++) {
      Vector3 p(x,y,0);
      float f = (std::abs(simplexNoise(p*0.1)))*255;
      data[(x+y*w)*3 + 0] = f;
      data[(x+y*w)*3 + 1] = f;
      data[(x+y*w)*3 + 2] = f;
    }
  }
  saveImage("test-abs.png", w,h,data);
  
  for (size_t x = 0; x<w; x++) {
    for (size_t y = 0; y<h; y++) {
      Vector3 p(x,y,0);
      p.x += simplexNoise(p*0.1 + Vector3(0,0, 1))*2;
      p.y += simplexNoise(p*0.1 + Vector3(0,0,-1))*2;
      
      float f = (std::abs(simplexNoise(p*0.1)))*255;
      data[(x+y*w)*3 + 0] = f;
      data[(x+y*w)*3 + 1] = f;
      data[(x+y*w)*3 + 2] = f;
    }
  }
  saveImage("test-abs-ofs.png", w,h,data);
  delete[] data;
  return 0;
  */
/*
  char abc[]= "abcdefghijklmnopqrstuvwxyz";
  for (size_t i=0; i<26*26*26*26; i++) {
    char tmp[5] = { 0 };
    size_t n = i;
    tmp [3] = (abc[n%26]); n/=26;
    tmp [2] = (abc[n%26]); n/=26;
    tmp [1] = (abc[n%26]); n/=26;
    tmp [0] = (abc[n%26]); n/=26;
    if (strchr(tmp, 'a') || strchr(tmp,'e') || strchr(tmp, 'i') || strchr(tmp, 'o') || strchr(tmp, 'u')) std::cerr << tmp << std::endl;
  }
return 0;
*/
  // Set up glfw
  if (!glfwInit()) {
    std::cerr << "Could not initialize GLFW\n";
    return -1;
  }

  if (!glfwOpenWindow(screenWidth, screenHeight, 8, 8, 8, 0, 24, 0, GLFW_WINDOW)) {
    std::cerr << "Could not open window\n";
    return -1;
  }

  glfwSetWindowPos(1920,0);
  glfwSetWindowSizeCallback(&resize);
  glfwSetMousePosCallback(&mouseMove);
  glfwSetMouseButtonCallback(&mouseClick);
  glfwSetKeyCallback(&keyEvent);
  glfwSwapInterval(1);

  // We'd like extensions with that
  GLeeInit();

  // Basic GL settings
  glCullFace(GL_BACK);
  glEnable(GL_CULL_FACE);
  
  glEnable(GL_ALPHA_TEST);
  glAlphaFunc(GL_GREATER, 0);
  
  glEnable(GL_TEXTURE_2D);
  glEnable(GL_SCISSOR_TEST);

  // Colors look nicer unclamped
  if (GLEE_ARB_color_buffer_float) {
    glClampColorARB(GL_CLAMP_VERTEX_COLOR_ARB, GL_FALSE);
    glClampColorARB(GL_CLAMP_FRAGMENT_COLOR_ARB, GL_FALSE);
  }

  // Light fog for the right mood
  float black[4] = { 0,0,0,1 };
  glEnable(GL_FOG);
  glFogfv(GL_FOG_COLOR, black);
  glFogf(GL_FOG_START, 0);
  glFogf(GL_FOG_END, 64);
  glHint(GL_FOG_HINT, GL_NICEST);

  updateScreenSize();

  // Create new game
  new Game("seed", 0);
  
  float lastT = glfwGetTime();
  while (glfwGetWindowParam(GLFW_OPENED)) {
    // render game
    Game::Instance->Render();

    // update game (at most 0.1s at a time)
    float t = glfwGetTime();
    while(t - lastT > 0.1) {
      lastT += 0.1;
      Game::Instance->Update(lastT, t - lastT);
    }
    Game::Instance->Update(t, t - lastT);
    lastT = t;

    // update input   
    if (mouseGrab && glfwGetKey(GLFW_KEY_ESC)) {
      glfwEnable(GLFW_MOUSE_CURSOR);
      mouseGrab = false;
    }
    if (guiActive) {
      glfwEnable(GLFW_MOUSE_CURSOR);
    }
    mouseDX = mouseDY = 0;

    /* Swap front and back buffers and process events */
    glfwSwapBuffers();

    updateTextures();
  }
  
  glfwCloseWindow();
  glfwTerminate();

  return 0;
}

#ifdef WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

int CALLBACK WinMain(
  HINSTANCE,
  HINSTANCE,
  LPSTR,
  int
) {
  return main();
}
#endif
