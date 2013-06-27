#include "gfx.h"
#include "game.h"

#include <png.h>
#include <zlib.h>

float Wave(float x, float z, float t, float a, float o) {
  return o + a * cos( ((x+z+t)*0.4 )*0.1) * cos( ((x-z  )*0.6+t*0.3) * 0.5) * 0.5;
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

int main() {
  std::cerr << credits() << std::endl;
  std::setlocale(LC_ALL, "en_US.utf8");

  // Initialize graphics or die
  new Gfx(Point(1920, 0), Point(320, 240), false);
  if (!Gfx::Instance->Init()) return -1;
  
  // Create new game
  new Game("seed", 0);
  
  float lastT = Gfx::Instance->GetTime();
  while (Gfx::Instance->Swap()) {
    // render game
    Game::Instance->Render();

    // update game (at most 0.1s at a time)
    float t = Gfx::Instance->GetTime();
    while(t - lastT > 0.1) {
      lastT += 0.1;
      Game::Instance->Update(lastT, t - lastT);
    }
    Game::Instance->Update(t, t - lastT);
    lastT = t;
  }

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
