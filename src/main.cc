#include "game.h"

#include <png.h>
#include <zlib.h>

float Wave(float x, float z, float t, float a, float o) {
  return o + a * cos( ((x+z+t)*0.4 )*0.1) * cos( ((x-z  )*0.6+t*0.3) * 0.5) * 0.5;
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
  std::cerr << credits() << std::endl;
  std::setlocale(LC_ALL, "en_US.utf8");

  new Game("seed", 0);
  if (!Game::Instance->Init()) return -1;

  while(Game::Instance->Frame()) 
    ;

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
