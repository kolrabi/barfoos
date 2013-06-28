#ifndef BARFOOS_GFX_H
#define BARFOOS_GFX_H

#include "common.h"

class Gfx final {
public:

  Gfx(const Point &pos, const Point &size, bool fullscreen);
  ~Gfx();
  static std::unique_ptr<Gfx> Instance;
  
  bool Init();
  void Deinit();
  
  const Point &GetScreenSize()        const { return screenSize; }
  const Point &GetVirtualScreenSize() const { return virtualScreenSize; }
  const Point &GetMousePos()          const { return mousePos; }
  
  void IncGuiCount();
  void DecGuiCount();
  
  bool Swap();
  float GetTime() const;

  void Viewport(const Rect &view);
  void View3D(const Vector3 &pos, const Vector3 &forward, float fovY = 60.0, const Vector3 &up = Vector3(0,1,0)) const;
  void ViewGUI() const;
  
  void SetTextureFrame(size_t currentFrame, size_t frameCount) const;

  void DrawTriangles(const std::vector<Vertex> &vertices) const;
  void DrawQuads(const std::vector<Vertex> &vertices) const;
  
  void DrawUnitCube(unsigned int tex = 0) const;
  void DrawAABB(const AABB &aabb, unsigned int tex = 0) const;
  void DrawSprite(const Sprite &sprite, const Vector3 &pos, bool billboard = true) const;
  void DrawIcon(const Sprite &sprite, const Point &pos, const Point &size = Point(32, 32)) const;
  
private:

  bool isInit;
  float startTime;

  // display
  Point screenPos;
  Point screenSize;
  bool isFullscreen;
  Point virtualScreenSize;
  Point viewportSize;

  // input
  Point mousePos;
  Point mouseDelta;
  bool mouseGrab;
  
  void OnResize(const Point &size);
  void OnMouseMove(const Point &pos);
  void OnMouseButton(int button, int event);
  void OnKey(int key, int event);
  
  // gui
  size_t guiActiveCount;
  
  // drawing
  std::vector<Vertex> cubeVerts;
  std::vector<Vertex> quadVerts;
};

unsigned int loadTexture(const std::string &name, unsigned int tex = 0);
void updateTextures();

void drawIcon(const Point &center, const Point &size, unsigned int tex, float u=0, float uw=1);  

Point alignBottomLeftScreen(const Point &size, int padding = 0);
Point alignBottomRightScreen(const Point &size, int padding = 0);
Point alignTopLeftScreen(const Point &size, int padding = 0);
Point alignTopRightScreen(const Point &size, int padding = 0);

#endif

