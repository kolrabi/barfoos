#include "common.h"

#include "gfx/gfx.h"
#include "game/game.h"
#include "io/input.h"
#include "util/image.h"

#include "GLee.h"
#include <GLFW/glfw3.h>

static InputKey MapMouseButton(int b) {
  InputKey key;

  switch(b) {
    case GLFW_MOUSE_BUTTON_LEFT:  key = InputKey::MouseLeft; break;
    case GLFW_MOUSE_BUTTON_RIGHT: key = InputKey::MouseRight; break;
    default:                      key = InputKey::Invalid;
  }
  return key;
}

static InputKey MapKey(int k) {
  InputKey key;

  switch(k) {
    case 'W':                 key = InputKey::Forward;         break;
    case 'S':                 key = InputKey::Backward;        break;
    case 'A':                 key = InputKey::Left;            break;
    case 'D':                 key = InputKey::Right;           break;
    case ' ':                 key = InputKey::Jump;            break;
    case GLFW_KEY_LEFT_SHIFT: key = InputKey::Sneak;           break;
    case 'E':                 key = InputKey::Use;             break;
    case GLFW_KEY_TAB:        key = InputKey::Inventory;       break;
    case GLFW_KEY_ESCAPE:     key = InputKey::Escape;          break;

    case '+':                 key = InputKey::MapZoomIn;       break;
    case '-':                 key = InputKey::MapZoomOut;      break;
    case GLFW_KEY_KP_ADD:     key = InputKey::MapZoomIn;       break;
    case GLFW_KEY_KP_SUBTRACT:key = InputKey::MapZoomOut;      break;

    case '1':                 key = InputKey::ElementFire;     break;
    case '2':                 key = InputKey::ElementWater;    break;
    case '3':                 key = InputKey::ElementAir;      break;
    case '4':                 key = InputKey::ElementEarth;    break;
    case '5':                 key = InputKey::ElementLife;     break;
    case '\\':                key = InputKey::ElementClear;    break;
    case 'Q':                 key = InputKey::CastSpell;       break;

    case GLFW_KEY_F1:         key = InputKey::DebugDie;        break;
    case GLFW_KEY_F2:         key = InputKey::DebugEntityAABB; break;
    case GLFW_KEY_F3:         key = InputKey::DebugWireframe;  break;
    case GLFW_KEY_F4:         key = InputKey::DebugNoclip;     break;
    case GLFW_KEY_F5:         key = InputKey::DebugScreenshot; break;
    case GLFW_KEY_F6:         key = InputKey::DebugLog;        break;
    default:                  key = InputKey::Invalid;
                              Log("Unknown key: %04x %c\n", k, k);
  }
  return key;
}

GfxScreen::GfxScreen(Gfx &gfx, const Point &pos, const Point &size, bool fullscreen) :
  gfx(gfx),
  screenPos(pos),
  screenSize(size),
  isFullscreen(fullscreen),
  virtualScreenSize(size),
  viewportSize(size),
  guiActiveCount(0),
  mousePos(0,0),
  lastMousePos(0,0),
  mouseDelta(0,0),
  mouseGrab(false)
{}

bool
GfxScreen::Init(Game &game) {
  // Create window
  this->window = glfwCreateWindow(
    this->screenSize.x,
    this->screenSize.y,
    "foobar",
    nullptr,
    nullptr
  );

  if (!this->window) {
    Log("Could not create window\n");
    glfwTerminate();
    return false;
  }

  if (this->screenPos.x != -1 || this->screenPos.y != -1) {
    glfwSetWindowPos(
      this->window,
      this->screenPos.x,
      this->screenPos.y
    );
  }

  glfwMakeContextCurrent(this->window);
  glfwSetWindowUserPointer(this->window, &game);

  // Setup event handlers ----------------------------------------------

  // Window resize
  glfwSetWindowSizeCallback( this->window, [](GLFWwindow *window, int w, int h) {
    if (w < 400 || h < 300) {
      if (w<400) w = 400;
      if (h<300) h = 300;
      glfwSetWindowSize(window, w, h);
      return;
    }
    Game &game = *reinterpret_cast<Game*>(glfwGetWindowUserPointer(window));
    Gfx  &gfx  = game.GetGfx();
    gfx.GetScreen().Resize(game, Point(w,h));
  } );

  // Mouse cursor movement
  glfwSetCursorPosCallback(  this->window, [](GLFWwindow *window, double x, double y) {
    Game &game = *reinterpret_cast<Game*>(glfwGetWindowUserPointer(window));
    Gfx  &gfx  = game.GetGfx();
    gfx.GetScreen().MouseMove(game ,x, y);
  } );

  // Mouse buttons
  glfwSetMouseButtonCallback(this->window, [](GLFWwindow *window, int b, int e, int) {
    Game &game = *reinterpret_cast<Game*>(glfwGetWindowUserPointer(window));
    Gfx  &gfx  = game.GetGfx();
    gfx.GetScreen().MouseButton(game, b, e);
  } );

  glfwSetKeyCallback(        this->window, [](GLFWwindow *window, int k, int, int e, int) {
    Game &game = *reinterpret_cast<Game*>(glfwGetWindowUserPointer(window));
    Gfx  &gfx  = game.GetGfx();
    gfx.GetScreen().Key(game, k, e);
  } );

  this->Viewport(Rect());
  return true;
}

void
GfxScreen::Deinit() {
  glfwSetWindowSizeCallback( this->window, nullptr);
  glfwSetCursorPosCallback(  this->window, nullptr);
  glfwSetMouseButtonCallback(this->window, nullptr);
  glfwSetKeyCallback(        this->window, nullptr);

  glfwDestroyWindow(this->window);
  this->window = nullptr;
}

void
GfxScreen::Update(Game &game) {
  glfwPollEvents();

  if (this->mouseGrab && !this->guiActiveCount) {
    // send relative coordinates
    game.GetInput().HandleEvent(InputEvent(InputEventType::MouseDelta, mouseDelta));
    // reset cursor to center
    //glfwSetCursorPos(window, screenSize.x/2, screenSize.y/2);
    //lastMousePos = Point(screenSize.x/2, screenSize.y/2);
    this->mouseDelta = Point();
  }

  if (game.GetInput().IsKeyDown(InputKey::DebugScreenshot)) {
    this->Save("screenshot.png");
  }
}

void
GfxScreen::Resize(Game &game, const Point &size) {
  // update (virtual) screen size
  this->screenSize = size;
  if (this->screenSize.x < 800 || this->screenSize.y < 600) {
    this->virtualScreenSize.x = this->screenSize.x;
    this->virtualScreenSize.y = this->screenSize.y;
  } else {
    this->virtualScreenSize.x = this->screenSize.x/2;
    this->virtualScreenSize.y = this->screenSize.y/2;
  }

  Log("%d %d -> %d %d\n", this->screenSize.x, this->screenSize.y, this->virtualScreenSize.x, this->virtualScreenSize.y);
  game.GetInput().HandleEvent(InputEvent(InputEventType::ScreenResize, size));
}

void
GfxScreen::MouseMove(Game &game, double x, double y) {
  if (this->guiActiveCount || !this->mouseGrab) {
    // map to virtual screen size
    Point mousePos(
      (x / this->screenSize.x) * this->virtualScreenSize.x,
      (y / this->screenSize.y) * this->virtualScreenSize.y
    );

    // save for later
    this->mousePos = mousePos;

    // send absolute coordinats
    game.GetInput().HandleEvent(InputEvent(InputEventType::MouseMove, mousePos));
  } else if (this->mouseGrab) {
    this->mouseDelta = this->mouseDelta + Point(x,y) - this->lastMousePos;
    this->lastMousePos = Point(x,y);
  }
}

void
GfxScreen::MouseButton(Game &game, int b, int e) {
  bool down = (e != GLFW_RELEASE);
  InputKey key = MapMouseButton(b);

  if (!this->mouseGrab && down && b == GLFW_MOUSE_BUTTON_LEFT) {
    // grab mouse on click
    if (!this->guiActiveCount) {
      glfwSetCursorPos(this->window, this->screenSize.x/2, this->screenSize.y/2);
      this->mousePos = this->lastMousePos = Point(this->screenSize.x/2, this->screenSize.y/2);
      glfwSetInputMode(this->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    } else {
      game.GetInput().HandleEvent(InputEvent(InputEventType::Key, this->mousePos, key, down));
    }
    this->mouseGrab = true;
  } else {
    // if already grabbed
    game.GetInput().HandleEvent(InputEvent(InputEventType::Key, this->mousePos, key, down));
  }
}

void
GfxScreen::Key(Game &game, int k, int e) {
  bool down = e != GLFW_RELEASE;
  InputKey key = MapKey(k);

  if (this->mouseGrab && down && key == InputKey::Escape) {
    // ungrab mouse on escape
    glfwSetInputMode(this->window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    this->mouseGrab = false;
  } else {
    game.GetInput().HandleEvent(InputEvent(InputEventType::Key, this->mousePos, key, down));
  }
}

void
GfxScreen::Viewport(const Rect &view) {
  if (this->screenSize.x < 800 || this->screenSize.y < 600) {
    this->virtualScreenSize.x = this->screenSize.x;
    this->virtualScreenSize.y = this->screenSize.y;
  } else {
    this->virtualScreenSize.x = this->screenSize.x/2;
    this->virtualScreenSize.y = this->screenSize.y/2;
  }

  if (view.size.x == 0 || view.size.y == 0) {
    glScissor(0,0, this->screenSize.x, this->screenSize.y);
    glViewport(0,0, this->screenSize.x, this->screenSize.y);
    this->viewportSize = this->screenSize;
  } else {
    glScissor(view.pos.x, view.pos.y + this->screenSize.y - view.size.y, view.size.x, view.size.y);
    glViewport(view.pos.x, view.pos.y + this->screenSize.y - view.size.y, view.size.x, view.size.y);
    this->viewportSize = view.size;
  }
}


bool
GfxScreen::Swap() {
  glfwSwapBuffers(this->window);
  glViewport(0, 0, this->screenSize.x, this->screenSize.y);
  return !glfwWindowShouldClose(this->window);
}

void
GfxScreen::Save(const std::string &name) {
  uint8_t *data = new uint8_t[screenSize.x*screenSize.y*3];
  glReadPixels(0,0,screenSize.x, screenSize.y, GL_RGB, GL_UNSIGNED_BYTE, data);
  Image(screenSize, data, false).Save(name);
  Log("%s saved\n", name.c_str());
}

void
GfxScreen::IncGuiCount() {
  guiActiveCount ++;
  glfwSetInputMode(this->window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void
GfxScreen::DecGuiCount() {
  if (!guiActiveCount) return;

  guiActiveCount--;

  if (!guiActiveCount && this->mouseGrab) {
    glfwSetCursorPos(this->window, screenSize.x/2, screenSize.y/2);
    mousePos = lastMousePos = Point(screenSize.x/2, screenSize.y/2);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  }
}

Point
GfxScreen::AlignBottomLeftScreen(const Point &size, int padding) {
  return Point(
                                (padding + size.x/2),
    this->virtualScreenSize.y - (padding + size.y/2) );
}

Point
GfxScreen::AlignBottomRightScreen(const Point &size, int padding) {
  return Point(
    this->virtualScreenSize.x - (padding + size.x/2),
    this->virtualScreenSize.y - (padding + size.y/2)
  );
}

Point
GfxScreen::AlignTopLeftScreen(const Point &size, int padding) {
  return Point(
                                (padding + size.x/2),
                                (padding + size.y/2)
  );
}

Point
GfxScreen::AlignTopRightScreen(const Point &size, int padding) {
  return Point(
    this->virtualScreenSize.x - (padding + size.x/2),
                                (padding + size.y/2)
  );
}
