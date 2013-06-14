#include "GLee.h"
#include <GL/glfw.h>

#include "common.h"
#include "util.h"
#include "world.h"
#include "player.h"
#include "game.h"

#include <cctype>

Game *game = nullptr;
size_t level = 0;

bool mouseGrab = false;
int screenWidth = 320;
int screenHeight = 240;
float mouseDX = 0;
float mouseDY = 0;

float Wave(float x, float z, float t, float a, float o) {
  return o + a * cos( ((x+z+t)*0.4 )*0.1) * cos( ((x-z  )*0.6+t*0.3) * 0.5) * 0.5;
}

void mouseMove(int x, int y) {
  if (mouseGrab) {
    mouseDX = (x-screenWidth/2)*0.005;
    mouseDY = (y-screenHeight/2)*0.005;
    glfwSetMousePos(screenWidth/2, screenHeight/2);
  }
}

void mouseClick(int button, int down) {
  if (game) {
    if (!mouseGrab && down && button == GLFW_MOUSE_BUTTON_LEFT) {
      glfwSetMousePos(screenWidth/2, screenHeight/2);
//      glfwDisable(GLFW_MOUSE_CURSOR);
      mouseGrab = true;
    } else {
      game->MouseClick(button, down);
    }
  }
}

void keyEvent(int key, int action) {
  if (key == GLFW_KEY_F12 && action == GLFW_PRESS) {
    delete game;
    game = new Game("seed", ++level);
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
}

int main() {
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
  float black[4] = { 0.1,0.1,0.2,1 };
  glEnable(GL_FOG);
  glFogfv(GL_FOG_COLOR, black);
  glHint(GL_FOG_HINT, GL_NICEST);

  // Create new game
  game = new Game("seed");
  
  float lastT = glfwGetTime();
  while (glfwGetWindowParam(GLFW_OPENED)) {
    // render game
    game->Render();

    // update game (at most 0.1s at a time)
    float t = glfwGetTime();
    while(t - lastT > 0.1) {
      lastT += 0.1;
      game->Update(lastT);
    }
    game->Update(t);
    lastT = t;

    // update input   
    if (mouseGrab && glfwGetKey(GLFW_KEY_ESC)) {
      glfwEnable(GLFW_MOUSE_CURSOR);
      mouseGrab = false;
    }
    mouseDX = mouseDY = 0;

    /* Swap front and back buffers and process events */
    glfwSwapBuffers();

    updateTextures();
  }

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

std::vector<Token> Tokenize(const std::string &text) {
  std::vector<Token> tokens;
  Token cur;
  
  size_t p = 0;
  char c = text[p];
  char lastC = ' ';
  
  bool inComment = false;
  
  size_t col = 0;
  size_t line = 1;
  
  while(c) {
    col ++;
    if (c == '#' && col == 1) {
      inComment = true;
    } else if (c == '\r') {
      // ignore
    } else if (c == '\n') {
      if (inComment) {
        inComment = false;
      } else {
        tokens.push_back(cur);
        cur.text = "";
      }
      col = 0;
      line ++;
    } else if (inComment) {
    } else if (isspace(c)) {
      tokens.push_back(cur);
      cur.text = "";
    } else {
      cur.text += c;
      if (isspace(lastC)) {
        cur.col = col;
        cur.line = line;
      }
    }
    p++;
    lastC = c;
    c = text[p];
  }
  return tokens;
}
