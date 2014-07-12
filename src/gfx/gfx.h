#ifndef BARFOOS_GFX_H
#define BARFOOS_GFX_H

#include "math/2d.h"
#include "util/icolor.h"

#include "gfx/gfxscreen.h"

#include <vector>
#include <unordered_map>

class Gfx final {
public:

  static const size_t MaxLights = 4;

  float           GetTime                 ()                      const;
  void            Update                  (Game &game);
  GfxView &       GetView                 ()                            { return *view; }
  GfxScreen &     GetScreen               ()                            { return this->screen; }

  bool            UseFixedFunction        ()                      const { return this->useFixedFunction; }
  const Texture * GetNoiseTexture         ()                      const { return noiseTex; }
  void            SetPlayer               (const Player *player);

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
  void            DrawSprite              (const Sprite &sprite, const Vector3 &pos, const Vector3 &axis);
  void            DrawIcon                (const Sprite &sprite, const Point &pos, const Point &size = Point(32, 32));
  void            DrawIconQuad            (const Point &pos, const Point &size = Point(32, 32));
  void            DrawStretched           (const Texture *tex, const Rect &src, const Rect &dest);

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
  GfxScreen screen;
  bool useFixedFunction;
  bool isInit;
  float startTime;

  const Player *player;

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

