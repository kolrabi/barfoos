#ifndef BARFOOS_GFX_H
#define BARFOOS_GFX_H

#include "common.h"

#include "2d.h"
#include "icolor.h"

#include <vector>
#include <unordered_map>

class Gfx final {
public:

  static const size_t MaxLights = 4;

  float           GetTime                 ()                      const;
  void            Update                  (Game &game);
  bool            Swap                    ();
  GfxView &       GetView                 ()                            { return *view; }
  
  bool            UseFixedFunction        ()                      const { return this->useFixedFunction; }
  const Texture * GetNoiseTexture         ()                      const { return noiseTex; }
  void            SetPlayer               (const Player *player);

  void            IncGuiCount             ();
  void            DecGuiCount             ();

  void            ClearColor              (const IColor &color)   const;
  void            ClearDepth              (float depth)           const;

  void            SetShader               (const std::string &shader);
  void            SetTextureFrame         (const Texture *texture, size_t stage = 0, size_t currentFrame = 0, size_t frameCount = 1);
  void            SetBlendNormal          ();
  void            SetBlendAdd             ();
  void            SetFog                  (float l, const IColor &color);
  void            SetColor                (const IColor &color, float alpha = 1.0);
  void            SetLight                (const IColor &color);
  void            SetBackfaceCulling      (bool cull);
  void            SetLights               (const std::vector<Vector3> &positions, const std::vector<IColor> &colors);

  void            DrawTriangles           (VertexBuffer &buffer, size_t first=0, size_t vertexCount=0);
  void            DrawQuads               (VertexBuffer &buffer, size_t first=0, size_t vertexCount=0);

  void            DrawUnitCube            ();
  void            DrawUnitQuad            ();
  void            DrawAABB                (const AABB &aabb);
  void            DrawSprite              (const Sprite &sprite, const Vector3 &pos, bool flip = false, bool billboard = true, float angleV=0.0);
  void            DrawIcon                (const Sprite &sprite, const Point &pos, const Point &size = Point(32, 32));
  void            DrawIconQuad            (const Point &pos, const Point &size = Point(32, 32));
  void            DrawStretched           (const Texture *tex, const Rect &src, const Rect &dest);

  // TODO: move to separate GfxScreen class
  void            SaveScreen              (const std::string &name);

  const Point &   GetScreenSize           ()                      const { return screenSize; }
  const Point &   GetVirtualScreenSize    ()                      const { return virtualScreenSize; }
  const Point &   GetMousePos             ()                      const { return mousePos; }

  void            Viewport                (const Rect &view);

  Point           AlignBottomLeftScreen   (const Point &size, int padding = 0);
  Point           AlignBottomRightScreen  (const Point &size, int padding = 0);
  Point           AlignTopLeftScreen      (const Point &size, int padding = 0);
  Point           AlignTopRightScreen     (const Point &size, int padding = 0);

private:

  friend class GfxView;
  friend class Game;

                  Gfx                     (const Point &pos, const Point &size, bool fullscreen);
                  ~Gfx                    ();

                  Gfx                     (const Gfx &)           = delete;
  Gfx &           operator=               (const Gfx &)           = delete;

  bool            Init(Game &game);
  void            Deinit();


  // management
  GLFWwindow *window;
  bool useFixedFunction;
  bool isInit;
  float startTime;

  const Player *player;

  // screen: TODO: move to separate class
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
  VertexBuffer *vb;
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
};

#endif

