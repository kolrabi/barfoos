#ifndef BARFOOS_GFX_H
#define BARFOOS_GFX_H

#include "common.h"
#include "icolor.h"

struct Vertex;
struct InputEvent;
class GLFWwindow;
class Shader;
class Game;
class Gfx;

class GfxView final {
public:

  void Look(const Vector3 &pos, const Vector3 &forward, float fovY = 60.0, const Vector3 &up = Vector3(0,1,0));
  void GUI();
  
  void Push();
  void Pop();
  void Translate(const Vector3 &p);
  void Scale(const Vector3 &p);
  void Rotate(float angle, const Vector3 &p);
  
private:

  Matrix4 proj;
  Matrix4 modelView;
  Matrix4 textureMatrix;
  std::vector<Matrix4> projStack;
  std::vector<Matrix4> viewStack;
  std::vector<Matrix4> textureStack;

  friend class Gfx;
  
  Gfx &gfx;
  GfxView(Gfx &gfx) : gfx(gfx) {}
  
  void SetUniforms(const Shader *shader) const;
};

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
  GfxView     &GetView()                    { return view; }
  
  void IncGuiCount();
  void DecGuiCount();
  
  void ClearColor(const IColor &color) const;
  void ClearDepth(float depth) const;
  bool Swap();

  void SetShader(const Shader *shader);
  void SetTextureFrame(const Texture *texture, size_t stage = 0, size_t currentFrame = 0, size_t frameCount = 1);
  void SetFog(float e, float l, const IColor &color);
  void SetColor(const IColor &color);
  
  const Texture *GetNoiseTexture() const { return noiseTex; }

  void Viewport(const Rect &view);
  
  void DrawTriangles(const std::vector<Vertex> &vertices);
  void DrawQuads(const std::vector<Vertex> &vertices);
  void DrawTriangles(unsigned int vbo, size_t first, size_t vertexCount);
  void DrawQuads(unsigned int vbo, size_t first, size_t vertexCount);
  
  void DrawUnitCube();
  void DrawAABB(const AABB &aabb);
  void DrawSprite(const Sprite &sprite, const Vector3 &pos, bool billboard = true);
  void DrawIcon(const Sprite &sprite, const Point &pos, const Point &size = Point(32, 32));

  Point AlignBottomLeftScreen(const Point &size, int padding = 0);
  Point AlignBottomRightScreen(const Point &size, int padding = 0);
  Point AlignTopLeftScreen(const Point &size, int padding = 0);
  Point AlignTopRightScreen(const Point &size, int padding = 0);
  
private:

  friend class GfxView;
  
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

  // render state
  const Shader *activeShader;
  GfxView view;
  IColor color;
  std::map<size_t, const Texture *> activeTextures;
  size_t activeTextureStage;
  const Vertex *activeVertexPointer;
  
  float fogExp2;
  float fogLin;
  IColor fogColor;
  
  void SetUniforms() const;
  void BindVertexPointer(const Vertex *ptr);
};

const Texture *noiseTexture(const Point &size, const Vector3 &scale = Vector3(1,1,1), const Vector3 &offset = Vector3());
const Texture *loadTexture(const std::string &name, const Texture * tex = nullptr);
void updateTextures();

#endif

