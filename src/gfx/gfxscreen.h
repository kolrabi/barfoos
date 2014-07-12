#ifndef BARFOOS_GFXSCREEN_H
#define BARFOOS_GFXSCREEN_H

#include "common.h"

#include "math/2d.h"
#include "util/icolor.h"

#include <vector>
#include <unordered_map>

class GfxScreen final {
public:

  bool            Swap                    ();
  void            Save                    (const std::string &name);

  void            IncGuiCount             ();
  void            DecGuiCount             ();

  const Point &   GetPosition             ()                      const { return this->screenPos; }
  const Point &   GetSize                 ()                      const { return screenSize; }
  const Point &   GetVirtualSize          ()                      const { return virtualScreenSize; }
  const Point &   GetMousePos             ()                      const { return mousePos; }

  float           GetAspect               ()                      const { return (float)this->viewportSize.x / (float)this->viewportSize.y; };

  void            Viewport                (const Rect &view);

  Point           AlignBottomLeftScreen   (const Point &size, int padding = 0);
  Point           AlignBottomRightScreen  (const Point &size, int padding = 0);
  Point           AlignTopLeftScreen      (const Point &size, int padding = 0);
  Point           AlignTopRightScreen     (const Point &size, int padding = 0);

private:

  friend class Gfx;

                  GfxScreen               (Gfx &gfx, const Point &pos, const Point &size, bool fullscreen);

  bool            Init(Game &game);
  void            Deinit();
  void            Update(Game &game);

  void            Resize                  (Game &game, const Point &size);
  void            MouseMove               (Game &game, double x, double y);
  void            MouseButton             (Game &game, int btn, int ev);
  void            Key                     (Game &game, int k, int ev);

  Gfx &           gfx;
  GLFWwindow *    window;

  Point           screenPos;
  Point           screenSize;
  bool            isFullscreen;
  Point           virtualScreenSize;
  Point           viewportSize;
  size_t          guiActiveCount;


  // input
  Point           mousePos, lastMousePos;
  Point           mouseDelta;
  bool            mouseGrab;
};

#endif

