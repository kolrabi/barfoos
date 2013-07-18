#include <GLFW/glfw3.h>

#include "game.h"

#include <png.h>
#include <zlib.h>

#include "2d.h"
#include "vector3.h"
#include <sstream>

float Wave(float x, float z, float t, float a) {
  return a * cos( ((x+z)*0.4+t*0.4) )
           * cos( ((x-z)*0.6+t*0.7) );
}

Point::operator std::string() const {
  std::stringstream str;
  str << "[" << x << ", " << y << "]";
  return str.str();
}

Vector3::operator std::string() const {
  std::stringstream str;
  str << "[" << x << ", " << y << ", " << z << "]";
  return str.str();
}
  
static std::string credits() {
  std::string str;

  str += " GLFW - An OpenGL framework Version 3.0.2";
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

int main() {
  std::setlocale(LC_ALL, "en_US.utf8");

  Log("%s", credits().c_str());
  
  // Set up glfw
  if (!glfwInit()) {
    Log("Could not initialize GLFW\n");
    return -1;
  }
  Log("GLFW initialized\n");

  Game *game = new Game("seed", 0);
  if (!game->Init()) {
    Log("Could not initialize game object\n");
    delete game;
    glfwTerminate();
    return -1;
  }

  Log("Game object initialized, entering mainloop\n");
  try {
    while(game->Frame()) 
      ;
  } catch (std::exception &e) {
    Log("Caught exception: %s\n", e.what());
  }
    
  Log("Shutting down\n");
  
  delete game;
  
  glfwTerminate();
  
  Profile::Dump();

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
