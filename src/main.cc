#include <GLFW/glfw3.h>

#include "game.h"

#include <png.h>
#include <zlib.h>

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

int main(int argc, char **argv) {
  std::setlocale(LC_ALL, "en_US.utf8");

  Log("%s", credits().c_str());
  
  // Set up glfw
  if (!glfwInit()) {
    Log("Could not initialize GLFW\n");
    return -1;
  }
  Log("GLFW initialized\n");

  Game *game = new Game();
  if (!game->Init()) {
    Log("Could not initialize game object\n");
    delete game;
    glfwTerminate();
    return -1;
  }
  
  Log("Game object initialized %p, new game\n", game);
  if (argc == 1)
    game->NewGame("fooobaaar");
  else
    game->NewGame(argv[1]);
  
  Log("entering mainloop\n");
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

extern int __argc;
extern char **__argv;

int CALLBACK WinMain(
  HINSTANCE,
  HINSTANCE,
  LPSTR,
  int
) {
  return main(__argc, __argv);
}
#endif
