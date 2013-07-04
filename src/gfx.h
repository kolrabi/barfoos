#ifndef BARFOOS_GFX_H
#define BARFOOS_GFX_H

#include "common.h"
#include "icolor.h"

struct Vertex;
struct InputEvent;
class GLFWwindow;
class Shader;
class Game;

class Gfx final {
public:

  Gfx(const Point &pos, const Point &size, bool fullscreen);
  ~Gfx();
  
  bool Init(Game &game);
  void Deinit();
  
  float GetTime() const;
  void Update(Game &game);
  
  const Point &GetScreenSize()        const { return screenSize; }
  const Point &GetVirtualScreenSize() const { return virtualScreenSize; }
  const Point &GetMousePos()          const { return mousePos; }
  
  void IncGuiCount();
  void DecGuiCount();
  
  void ClearColor(const IColor &color) const;
  void ClearDepth(float depth) const;
  bool Swap();

  void Viewport(const Rect &view);
  void View3D(const Vector3 &pos, const Vector3 &forward, float fovY = 60.0, const Vector3 &up = Vector3(0,1,0)) const;
  void ViewGUI() const;
  
  void ViewPush() const;
  void ViewPop() const;
  void ViewTranslate(const Vector3 &p) const;
  void ViewScale(const Vector3 &p) const;
  void ViewRotate(float angle, const Vector3 &p) const;
  
  void SetDepthTest(bool on) const;
  void SetCullFace(bool on) const;
  void SetShader(const Shader *shader) const;
  void SetTextureFrame(const Texture *texture, size_t stage = 0, size_t currentFrame = 0, size_t frameCount = 1) const;
  void SetFog(float e, float l, const IColor &color);
  
  const Texture *GetNoiseTexture() const { return noiseTex; }
  void SetColor(const IColor &color) const;

  void DrawTriangles(const std::vector<Vertex> &vertices) const;
  void DrawQuads(const std::vector<Vertex> &vertices) const;
  
  void DrawUnitCube() const;
  void DrawAABB(const AABB &aabb) const;
  void DrawSprite(const Sprite &sprite, const Vector3 &pos, bool billboard = true) const;
  void DrawIcon(const Sprite &sprite, const Point &pos, const Point &size = Point(32, 32)) const;

  Point AlignBottomLeftScreen(const Point &size, int padding = 0);
  Point AlignBottomRightScreen(const Point &size, int padding = 0);
  Point AlignTopLeftScreen(const Point &size, int padding = 0);
  Point AlignTopRightScreen(const Point &size, int padding = 0);
  
private:

  GLFWwindow *window;
  
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
  
  // gui
  size_t guiActiveCount;
  
  // drawing
  const Texture *noiseTex;
  std::vector<Vertex> cubeVerts;
  std::vector<Vertex> quadVerts;
  
  float fogExp2 = 0;
  float fogLin  = 0;
  IColor fogColor;
};

const Texture *noiseTexture(const Point &size, const Vector3 &scale = Vector3(1,1,1), const Vector3 &offset = Vector3());
const Texture *loadTexture(const std::string &name, const Texture * tex = nullptr);
void updateTextures();

void drawIcon(const Point &center, const Point &size, unsigned int tex, float u=0, float uw=1);  

#endif

