#ifndef BARFOOS_GFX_H
#define BARFOOS_GFX_H

#include "common.h"

#include "2d.h"
#include "icolor.h"
#include "matrix4.h"

#include <unordered_map>

class GfxView;

class Gfx final {
public:

  Gfx(const Point &pos, const Point &size, bool fullscreen);
  Gfx(const Gfx &) = delete;
  Gfx &operator=(const Gfx &) = delete;
  ~Gfx();

  bool Init(Game &game);
  void Deinit();

  float GetTime() const;
  void Update(Game &game);
  void SaveScreen(const std::string &name);

  const Point &GetScreenSize()        const { return screenSize; }
  const Point &GetVirtualScreenSize() const { return virtualScreenSize; }
  const Point &GetMousePos()          const { return mousePos; }
  GfxView     &GetView()                    { return *view; }

  void IncGuiCount();
  void DecGuiCount();

  void ClearColor(const IColor &color) const;
  void ClearDepth(float depth) const;
  bool Swap();

  void SetShader(const std::string &shader);
  void SetTextureFrame(const Texture *texture, size_t stage = 0, size_t currentFrame = 0, size_t frameCount = 1);
  void SetBlendNormal();
  void SetBlendAdd();
  void SetFog(float l, const IColor &color);
  void SetColor(const IColor &color, float alpha = 1.0);
  void SetLight(const IColor &color);
  void SetBackfaceCulling(bool cull);
  void SetLights(const std::vector<Vector3> &positions, const std::vector<IColor> &colors);
  void SetPlayer(const Player *player);

  const Texture *GetNoiseTexture() const { return noiseTex; }

  void Viewport(const Rect &view);

  void DrawTriangles(const std::vector<Vertex> &vertices);
  void DrawQuads(const std::vector<Vertex> &vertices);
  void DrawTriangles(unsigned int vbo, size_t first, size_t vertexCount);
  void DrawQuads(unsigned int vbo, size_t first, size_t vertexCount);

  void DrawUnitCube();
  void DrawUnitQuad();
  void DrawAABB(const AABB &aabb);
  void DrawSprite(const Sprite &sprite, const Vector3 &pos, bool flip = false, bool billboard = true, float angleV=0.0);
  void DrawIcon(const Sprite &sprite, const Point &pos, const Point &size = Point(32, 32));
  void DrawIconQuad(const Point &pos, const Point &size = Point(32, 32));
  void DrawStretched(const Texture *tex, const Rect &src, const Rect &dest);

  Point AlignBottomLeftScreen(const Point &size, int padding = 0);
  Point AlignBottomRightScreen(const Point &size, int padding = 0);
  Point AlignTopLeftScreen(const Point &size, int padding = 0);
  Point AlignTopRightScreen(const Point &size, int padding = 0);

  static const size_t MaxLights = 4;

private:

  friend class GfxView;

  GLFWwindow *window;

  bool isInit;
  float startTime;
  unsigned int vbo;

  const Player *player;

  // display
  Point screenPos;
  Point screenSize;
  bool isFullscreen;
  Point virtualScreenSize;
  Point viewportSize;

  // input
  Point mousePos, lastMousePos;
  Point mouseDelta;
  bool mouseGrab;

  // gui
  size_t guiActiveCount;

  // drawing
  const Texture *noiseTex;
  std::vector<Vertex> cubeVerts;
  std::vector<Vertex> quadVerts;

  // render state
  std::unordered_map<std::string, std::shared_ptr<Shader>> shaders;
  std::shared_ptr<Shader> activeShader;
  GfxView *view;
  IColor color, light;
  float alpha;
  std::unordered_map<size_t, const Texture *> activeTextures;
  size_t activeTextureStage;
  const Vertex *activeVertexPointer;

  float fogLin;
  IColor fogColor;
  std::vector<Vector3> lightPositions;
  std::vector<IColor> lightColors;

  void SetUniforms() const;
  void BindVertexPointer(const Vertex *ptr);
};

#endif

