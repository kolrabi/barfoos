#include "GLee.h"
#include <GLFW/glfw3.h>

#include "gfx.h"
#include "game.h"
#include "vertex.h"
#include "shader.h"
#include "input.h"
#include "texture.h"
#include "player.h"
#include "sprite.h"
#include "vertex.h"

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
    
    case GLFW_KEY_F1:         key = InputKey::DebugDie;        break;
    case GLFW_KEY_F2:         key = InputKey::DebugEntityAABB; break;
    case GLFW_KEY_F3:         key = InputKey::DebugWireframe;  break;
    case GLFW_KEY_F4:         key = InputKey::DebugNoclip;     break;
    case GLFW_KEY_F5:         key = InputKey::DebugScreenshot; break;
    default:                  key = InputKey::Invalid;
  }
  return key;
}

Gfx::Gfx(const Point &pos, const Point &size, bool fullscreen) : 
  window(nullptr),
  isInit(false),
  startTime(glfwGetTime()),
  player(nullptr),
  
  screenPos(pos),
  screenSize(size),
  isFullscreen(fullscreen),
  virtualScreenSize(size),
  viewportSize(size),
  
  mousePos(),
  mouseDelta(),
  mouseGrab(false),
  guiActiveCount(0),
  
  noiseTex(nullptr),
  cubeVerts(0),
  quadVerts(0),

  activeShader(nullptr),
  view(*this),
  color(255, 255, 255),
  alpha(1.0),
  activeTextures(),
  activeTextureStage(0),
  activeVertexPointer(nullptr),
  
  fogLin(0.05),
  fogColor(64, 64, 64),
  
  lightPositions(MaxLights),
  lightColors(MaxLights)
{
  // unit cube vertices
  this->cubeVerts.push_back(Vertex(Vector3( 1, 1, -1), IColor(255,255,255), 0,1, Vector3( 1, 0, 0)));
  this->cubeVerts.push_back(Vertex(Vector3( 1, 1,  1), IColor(255,255,255), 1,1, Vector3( 1, 0, 0)));
  this->cubeVerts.push_back(Vertex(Vector3( 1,-1,  1), IColor(255,255,255), 1,0, Vector3( 1, 0, 0)));
  this->cubeVerts.push_back(Vertex(Vector3( 1,-1, -1), IColor(255,255,255), 0,0, Vector3( 1, 0, 0)));

  this->cubeVerts.push_back(Vertex(Vector3(-1,-1, -1), IColor(255,255,255), 1,0, Vector3(-1, 0, 0)));
  this->cubeVerts.push_back(Vertex(Vector3(-1,-1,  1), IColor(255,255,255), 2,0, Vector3(-1, 0, 0)));
  this->cubeVerts.push_back(Vertex(Vector3(-1, 1,  1), IColor(255,255,255), 2,1, Vector3(-1, 0, 0)));
  this->cubeVerts.push_back(Vertex(Vector3(-1, 1, -1), IColor(255,255,255), 1,1, Vector3(-1, 0, 0)));
  
  this->cubeVerts.push_back(Vertex(Vector3(-1,-1,  1), IColor(255,255,255), 2,0, Vector3( 0, 0, 1)));
  this->cubeVerts.push_back(Vertex(Vector3( 1,-1,  1), IColor(255,255,255), 3,0, Vector3( 0, 0, 1)));
  this->cubeVerts.push_back(Vertex(Vector3( 1, 1,  1), IColor(255,255,255), 3,1, Vector3( 0, 0, 1)));
  this->cubeVerts.push_back(Vertex(Vector3(-1, 1,  1), IColor(255,255,255), 2,1, Vector3( 0, 0, 1)));

  this->cubeVerts.push_back(Vertex(Vector3(-1, 1, -1), IColor(255,255,255), 3,1, Vector3( 0, 0,-1)));
  this->cubeVerts.push_back(Vertex(Vector3( 1, 1, -1), IColor(255,255,255), 4,1, Vector3( 0, 0,-1)));
  this->cubeVerts.push_back(Vertex(Vector3( 1,-1, -1), IColor(255,255,255), 4,0, Vector3( 0, 0,-1)));
  this->cubeVerts.push_back(Vertex(Vector3(-1,-1, -1), IColor(255,255,255), 3,0, Vector3( 0, 0,-1)));
  
  this->cubeVerts.push_back(Vertex(Vector3(-1, 1,  1), IColor(255,255,255), 4,0, Vector3( 0, 1, 0)));
  this->cubeVerts.push_back(Vertex(Vector3( 1, 1,  1), IColor(255,255,255), 5,0, Vector3( 0, 1, 0)));
  this->cubeVerts.push_back(Vertex(Vector3( 1, 1, -1), IColor(255,255,255), 5,1, Vector3( 0, 1, 0)));
  this->cubeVerts.push_back(Vertex(Vector3(-1, 1, -1), IColor(255,255,255), 4,1, Vector3( 0, 1, 0)));
  
  this->cubeVerts.push_back(Vertex(Vector3(-1,-1, -1), IColor(255,255,255), 5,1, Vector3( 0,-1, 0)));
  this->cubeVerts.push_back(Vertex(Vector3( 1,-1, -1), IColor(255,255,255), 6,1, Vector3( 0,-1, 0)));
  this->cubeVerts.push_back(Vertex(Vector3( 1,-1,  1), IColor(255,255,255), 6,0, Vector3( 0,-1, 0)));
  this->cubeVerts.push_back(Vertex(Vector3(-1,-1,  1), IColor(255,255,255), 5,0, Vector3( 0,-1, 0)));

  // quad
  this->quadVerts.push_back(Vertex(Vector3(-1,-1,  0), IColor(255,255,255), 0,0, Vector3( 0, 0, 0)));
  this->quadVerts.push_back(Vertex(Vector3( 1,-1,  0), IColor(255,255,255), 1,0, Vector3( 0, 0, 0)));
  this->quadVerts.push_back(Vertex(Vector3( 1, 1,  0), IColor(255,255,255), 1,1, Vector3( 0, 0, 0)));
  this->quadVerts.push_back(Vertex(Vector3(-1, 1,  0), IColor(255,255,255), 0,1, Vector3( 0, 0, 0)));
}

Gfx::~Gfx() {
  if (isInit) {
    this->Deinit();
  }
}

bool 
Gfx::Init(Game &game) {
  Log("Initializing GFX\n");

  // Create window
  this->window = glfwCreateWindow(screenSize.x, screenSize.y, "foobar", NULL, NULL);
  if (!this->window) {
    Log("Could not create window\n");
    glfwTerminate();
    return false;
  }
  
  if (this->screenPos.x != -1 || this->screenPos.y != -1)
    glfwSetWindowPos(this->window, this->screenPos.x, this->screenPos.y);
    
  glfwMakeContextCurrent(this->window);
  
  glfwSetWindowUserPointer(this->window, &game);
  
  if (this->screenSize.x <= 640) {
    this->virtualScreenSize.x = this->screenSize.x;
    this->virtualScreenSize.y = this->screenSize.y;
  } else {
    this->virtualScreenSize.x = this->screenSize.x/2;
    this->virtualScreenSize.y = this->screenSize.y/2;
  }
  this->Viewport(Rect(Point(), this->screenSize));
  
  // Setup event handlers ----------------------------------------------

  // Window resize
  glfwSetWindowSizeCallback( this->window, [](GLFWwindow *window, int w, int h) { 
    if (w < 320 || h < 240) {
      if (w<320) w = 320;
      if (h<240) h = 240;
      glfwSetWindowSize(window, w, h);
      return;
    }
    Game &game = *reinterpret_cast<Game*>(glfwGetWindowUserPointer(window));
    Gfx  &gfx  = game.GetGfx();
    
    Point size(w,h);
    
    // update (virtual) screen size
    gfx.screenSize = size;
    if (gfx.screenSize.x <= 640) {
      gfx.virtualScreenSize.x = gfx.screenSize.x;
      gfx.virtualScreenSize.y = gfx.screenSize.y;
    } else {
      gfx.virtualScreenSize.x = gfx.screenSize.x/2;
      gfx.virtualScreenSize.y = gfx.screenSize.y/2;
    }
    
    game.GetInput().HandleEvent(InputEvent(InputEventType::ScreenResize, size));
  } );
  
  // Mouse cursor movement
  glfwSetCursorPosCallback(  this->window, [](GLFWwindow *window, double x, double y) { 
    Game &game = *reinterpret_cast<Game*>(glfwGetWindowUserPointer(window));
    Gfx  &gfx  = game.GetGfx();
    
    if (gfx.guiActiveCount || !gfx.mouseGrab) {
      // map to virtual screen size
      Point mousePos(
        (x / gfx.screenSize.x) * gfx.virtualScreenSize.x,
        (y / gfx.screenSize.y) * gfx.virtualScreenSize.y
      );
      
      // save for later
      gfx.mousePos = mousePos;
    
      // send absolute coordinats
      game.GetInput().HandleEvent(InputEvent(InputEventType::MouseMove, mousePos)); 
    } else if (gfx.mouseGrab) {
      // get distance from center
      gfx.mouseDelta = gfx.mouseDelta + Point(
        x - gfx.screenSize.x/2,
        y - gfx.screenSize.y/2
      );
      // reset cursor to center
      glfwSetCursorPos(gfx.window, gfx.screenSize.x/2, gfx.screenSize.y/2);
    }
  } );
  
  // Mouse buttons
  glfwSetMouseButtonCallback(this->window, [](GLFWwindow *window, int b, int e, int) { 
    Game &game = *reinterpret_cast<Game*>(glfwGetWindowUserPointer(window));
    Gfx  &gfx  = game.GetGfx();
    
    bool down = (e != GLFW_RELEASE);
    InputKey key = MapMouseButton(b);
  
    if (!gfx.mouseGrab && down && b == GLFW_MOUSE_BUTTON_LEFT) {
      // grab mouse on click
      if (!gfx.guiActiveCount) {
        glfwSetCursorPos(gfx.window, gfx.screenSize.x/2, gfx.screenSize.y/2);
        glfwSetInputMode(gfx.window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
      } else {
        game.GetInput().HandleEvent(InputEvent(InputEventType::Key, gfx.mousePos, key, down));
      }
      gfx.mouseGrab = true;
    } else {
      // if already grabbed
      game.GetInput().HandleEvent(InputEvent(InputEventType::Key, gfx.mousePos, key, down));
    }
  } );
  
  glfwSetKeyCallback(        this->window, [](GLFWwindow *window, int k, int, int e, int) { 
    Game &game = *reinterpret_cast<Game*>(glfwGetWindowUserPointer(window));
    Gfx  &gfx  = game.GetGfx();
    
    bool down = e != GLFW_RELEASE;
    InputKey key = MapKey(k);
  
    if (gfx.mouseGrab && down && key == InputKey::Escape) {
      // ungrab mouse on escape
      glfwSetInputMode(gfx.window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
      gfx.mouseGrab = false;
    } else {
      game.GetInput().HandleEvent(InputEvent(InputEventType::Key, gfx.mousePos, key, down));
    }
  } );
  
  //
  //glfwSwapInterval(1);

  // We'd like extensions with that
  GLeeInit();
  
  // Basic GL settings
  glCullFace(GL_BACK);
  glEnable(GL_CULL_FACE);
  
  //glEnable(GL_ALPHA_TEST);
  //glAlphaFunc(GL_GREATER, 0);
  
  glEnable(GL_TEXTURE_2D);
  glEnable(GL_SCISSOR_TEST);
 
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glEnable(GL_BLEND);
  SetBlendNormal();
  
 // Colors look nicer unclamped
  if (GLEE_ARB_color_buffer_float) {
    glClampColorARB(GL_CLAMP_VERTEX_COLOR_ARB, GL_FALSE);
    glClampColorARB(GL_CLAMP_FRAGMENT_COLOR_ARB, GL_FALSE);
  }

  this->noiseTex = noiseTexture(Point(256,256), Vector3(32,32,32));
  SetTextureFrame(this->noiseTex, 1);

  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_COLOR_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  
  this->BindVertexPointer(&this->quadVerts[0]);
    
  isInit = true;
  return true;
}

void
Gfx::Deinit() {
  glfwSetWindowSizeCallback( this->window, nullptr);
  glfwSetCursorPosCallback(  this->window, nullptr);
  glfwSetMouseButtonCallback(this->window, nullptr);
  glfwSetKeyCallback(        this->window, nullptr);
  
  glfwDestroyWindow(this->window);
}

float 
Gfx::GetTime() const {
  return glfwGetTime();
}

void 
Gfx::IncGuiCount() {
  guiActiveCount ++;
  glfwSetInputMode(this->window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void
Gfx::DecGuiCount() {
  if (!guiActiveCount) return;
  
  guiActiveCount--;
  
  if (!guiActiveCount && mouseGrab) {
    glfwSetCursorPos(this->window, screenSize.x/2, screenSize.y/2);
    glfwSetInputMode(this->window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
  }
}

void 
Gfx::ClearColor(const IColor &color) const {
  glClearColor(color.r/255.0, color.g/255.0, color.b/255.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT);
}

void 
Gfx::ClearDepth(float depth) const {
  glClearDepth(depth);
  glClear(GL_DEPTH_BUFFER_BIT);
}

void 
Gfx::Update(Game &game) {
  glfwPollEvents();
  updateTextures();

  if (game.GetInput().IsKeyActive(InputKey::DebugWireframe)) {
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  } else {
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  }
  
  if (this->mouseGrab && !this->guiActiveCount) { 
    // send relative coordinates
    game.GetInput().HandleEvent(InputEvent(InputEventType::MouseDelta, mouseDelta)); 

    mouseDelta = Point();
  }

  if (game.GetInput().IsKeyDown(InputKey::DebugScreenshot)) {
    SaveScreen("screenshot.png");
  }
}

void
Gfx::SaveScreen(const std::string &name) {
  uint8_t *data = new uint8_t[screenSize.x*screenSize.y*3];
  glReadPixels(0,0,screenSize.x, screenSize.y, GL_RGB, GL_UNSIGNED_BYTE, data);
  saveImage(name, screenSize.x, screenSize.y, data); 
  delete [] data;
  Log("%s saved\n", name.c_str());
}

bool
Gfx::Swap() {
  glfwSwapBuffers(this->window);
  glViewport(0, 0, this->screenSize.x, this->screenSize.y);
  
  return !glfwWindowShouldClose(this->window);
}

void Gfx::Viewport(const Rect &view) {
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

void Gfx::SetShader(const std::string &name) {
  if (name == "") {
    glUseProgramObjectARB(0);
    this->activeShader = nullptr;
    return;
  }
  
  if (!shaders[name]) shaders[name] = std::shared_ptr<Shader>(new Shader(name));
  this->activeShader = shaders[name];

  glUseProgramObjectARB(shaders[name]->GetProgram());
}

void Gfx::SetBlendNormal() {
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
}

void Gfx::SetBlendAdd() {
  glBlendFunc(GL_ONE, GL_ONE);
}

void 
Gfx::SetFog(float l, const IColor &color) {
  this->fogLin = l;
  this->fogColor = color;
}

void 
Gfx::SetLights(const std::vector<Vector3> &positions, const std::vector<IColor> &colors) {
  this->lightPositions = positions;
  this->lightColors = colors; 
}

void 
Gfx::SetTextureFrame(const Texture *texture, size_t stage, size_t currentFrame, size_t frameCount) {
  if (stage != this->activeTextureStage) {
    glActiveTexture(GL_TEXTURE0 + stage);
    this->activeTextureStage = stage;
  }

  if (this->activeTextures[stage] != texture) {
    if (texture) {
      glBindTexture(GL_TEXTURE_2D, texture->handle);
      glEnable(GL_TEXTURE_2D);
    } else {
      glBindTexture(GL_TEXTURE_2D, 0);
      glDisable(GL_TEXTURE_2D);
    }
    this->activeTextures[stage] = texture;
  }
  
  if (!texture) return;
  
  this->view.textureStack.back() = Matrix4();
  
  if (frameCount > 1) {
    this->view.textureStack.back() = 
      Matrix4::Scale(Vector3(1.0/frameCount, 1, 1)) *
      Matrix4::Translate(Vector3(currentFrame, 0, 0));
  }
}

void
Gfx::SetColor(const IColor &color, float alpha) {
  this->color = color;
  this->alpha = alpha;
}

void 
Gfx::SetBackfaceCulling(bool cull) {
  if (cull) {
    glEnable(GL_CULL_FACE);
  } else {
    glDisable(GL_CULL_FACE);
  }
}

void 
Gfx::SetPlayer(const Player *player) {
  this->player = player;
}

void 
Gfx::SetUniforms() const {
  if (!this->activeShader) return;

  std::vector<Vector3> lightPos;
  std::vector<IColor>  lightCol;

  for (size_t i=0; i<this->lightPositions.size(); i++) {
//    if (this->view.IsPointVisible(this->lightPositions[i])) {
      lightPos.push_back(this->lightPositions[i]);
      lightCol.push_back(this->lightColors[i]);
//    }
  }

  lightCol.resize(MaxLights, IColor(0,0,0));
  lightPos.resize(MaxLights);
  
  this->view.SetUniforms(this->activeShader);
  
  this->activeShader->Uniform("u_fogLin",   this->fogLin);
  this->activeShader->Uniform("u_fogColor", this->fogColor);
  this->activeShader->Uniform("u_time",     this->GetTime());
  this->activeShader->Uniform("u_color",    this->color, this->alpha);
  
  this->activeShader->Uniform("u_lightPos",   lightPos);
  this->activeShader->Uniform("u_lightColor", lightCol);
  
  this->activeShader->Uniform("u_texture", 0);
  this->activeShader->Uniform("u_texture2", 1);
  
  if (this->player) this->player->SetUniforms(this->activeShader);
}

void
Gfx::BindVertexPointer(const Vertex *ptr) {
  //if (ptr == this->activeVertexPointer) return;
  
  glInterleavedArrays(GL_T2F_C4F_N3F_V3F,  sizeof(Vertex), ptr);
  this->activeVertexPointer = ptr;
}

void 
Gfx::DrawTriangles(const std::vector<Vertex> &vertices) {
  if (vertices.empty()) return;
  this->SetUniforms();
  this->BindVertexPointer(&vertices[0]);
  glDrawArrays(GL_TRIANGLES, 0, vertices.size());
}

void 
Gfx::DrawQuads(const std::vector<Vertex> &vertices) { 
  if (vertices.empty()) return;
  this->SetUniforms();
  this->BindVertexPointer(&vertices[0]);
  glDrawArrays(GL_QUADS, 0, vertices.size());
}

void 
Gfx::DrawTriangles(unsigned int vbo, size_t first, size_t vertexCount) {
  if (!vertexCount) return;

  this->SetUniforms();

  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  this->BindVertexPointer(nullptr);
  glDrawArrays(GL_TRIANGLES,    first, vertexCount);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void 
Gfx::DrawQuads(unsigned int vbo, size_t first, size_t vertexCount) {
  if (!vertexCount) return;

  this->SetUniforms();
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  this->BindVertexPointer(nullptr);
  glDrawArrays(GL_QUADS,        first, vertexCount);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Gfx::DrawUnitCube() {
  this->DrawQuads(this->cubeVerts);
}    

void Gfx::DrawAABB(const AABB &aabb) {
  this->view.Push();
  this->view.Translate(aabb.center);
  this->view.Scale(aabb.extents);
  this->DrawUnitCube();
  this->view.Pop();
}

void Gfx::DrawSprite(const Sprite &sprite, const Vector3 &pos, bool flip, bool billboard) {
  this->view.Push();
  this->view.Translate(pos);

  if (billboard) {
    this->view.Billboard(flip, sprite.vertical);
    this->view.Translate(Vector3(sprite.offsetX, sprite.offsetY, 0));
  }
  
  this->view.Scale(Vector3(sprite.width/2, sprite.height/2, 1));
  SetBackfaceCulling(!flip); 

  if (sprite.texture) {
    this->SetTextureFrame(sprite.texture, 0, sprite.currentFrame, sprite.totalFrames);
    this->DrawQuads(this->quadVerts);
  }
  if (sprite.emissiveTexture) {
    this->SetTextureFrame(sprite.emissiveTexture, 0, sprite.currentFrame, sprite.totalFrames);
    this->SetBlendAdd();
    this->DrawQuads(this->quadVerts);
    this->SetBlendNormal();
  }
  SetBackfaceCulling(true); 

  this->view.Pop();
}

void Gfx::DrawIcon(const Sprite &sprite, const Point &center, const Point &size) {
  this->view.Push();
  this->view.Translate(Vector3(center.x + sprite.offsetX*size.x, center.y + sprite.offsetY*size.y, 0));
  this->view.Scale    (Vector3(size.x/2, -size.y/2, 1));
  if (sprite.texture) {
    this->SetTextureFrame(sprite.texture, 0, sprite.currentFrame, sprite.totalFrames);
    this->DrawQuads(this->quadVerts);
  }
  if (sprite.emissiveTexture) {
    this->SetTextureFrame(sprite.emissiveTexture, 0, sprite.currentFrame, sprite.totalFrames);
    this->SetBlendAdd();
    this->DrawQuads(this->quadVerts);
    this->SetBlendNormal();
  }
  this->view.Pop();
}

void Gfx::DrawIconQuad(const Point &center, const Point &size) {
  this->view.Push();
  this->view.Translate(Vector3(center.x, center.y, 0));
  this->view.Scale    (Vector3(size.x/2, -size.y/2, 1));
  this->DrawQuads(this->quadVerts);
  this->view.Pop();
}

void Gfx::DrawStretched(const Texture *tex, const Rect &src, const Rect &dest) {
  if (!tex) return;
  
  float u1 = float(src.pos.x)/tex->size.x;
  float u2 = float(src.pos.x + src.size.x)/tex->size.x;
  float v1 = float(src.pos.y)/tex->size.y;
  float v2 = float(src.pos.y + src.size.y)/tex->size.y;
  
  std::vector<Vertex> verts;
  verts.push_back(Vertex(Vector3(dest.pos.x,             dest.pos.y+dest.size.y,  0), IColor(255,255,255), u1, v1, Vector3( 0, 0, 1)));
  verts.push_back(Vertex(Vector3(dest.pos.x+dest.size.x, dest.pos.y+dest.size.y,  0), IColor(255,255,255), u2, v1, Vector3( 0, 0, 1)));
  verts.push_back(Vertex(Vector3(dest.pos.x+dest.size.x, dest.pos.y,              0), IColor(255,255,255), u2, v2, Vector3( 0, 0, 1)));
  verts.push_back(Vertex(Vector3(dest.pos.x,             dest.pos.y,              0), IColor(255,255,255), u1, v2, Vector3( 0, 0, 1)));
  
  DrawQuads(verts);
}

/*void Gfx::DrawSubTex(const Point &pos, const Point &size, const Point &srcPos, const Texture *tex) {
  this->SetTextureFrame(tex);
  
  float u = float(srcPos.x)  / float(tex->size.x);
  float v = float(srcPos.y)  / float(tex->size.y);
  float w = float(srcSize.x) / float(tex->size.x);
  float h = float(srcSize.y) / float(tex->size.y);
  
  std::vector<Vertex> verts;
  verts.push_back(Vertex(Vector3(pos.x,        pos.y+size.y,  0), IColor(255,255,255), u,   v,   Vector3( 0, 0, 1)));
  verts.push_back(Vertex(Vector3(pos.x+size.x, pos.y+size.y,  0), IColor(255,255,255), u+w, v,   Vector3( 0, 0, 1)));
  verts.push_back(Vertex(Vector3(pos.x+size.x, pos.y,         0), IColor(255,255,255), u+w, v+h, Vector3( 0, 0, 1)));
  verts.push_back(Vertex(Vector3(pos.x,        pos.y,         0), IColor(255,255,255), u,   v+h, Vector3( 0, 0, 1)));
  
  DrawQuads(verts);
}
*/
/*
void Gfx::DrawNinePatch(const NinePatch &patch, const Rect &rect) {
}
*/

Point 
Gfx::AlignBottomLeftScreen(const Point &size, int padding) {
  const Point &ssize(this->GetVirtualScreenSize());
  return Point( padding + size.x/2, ssize.y - padding - size.y/2 );
}

Point 
Gfx::AlignBottomRightScreen(const Point &size, int padding) {
  const Point &ssize(this->GetVirtualScreenSize());
  return Point( ssize.x - padding - size.x/2, ssize.y - padding - size.y/2 );
}

Point 
Gfx::AlignTopLeftScreen(const Point &size, int padding) {
  return Point( padding + size.x/2, padding + size.y/2 );
}

Point 
Gfx::AlignTopRightScreen(const Point &size, int padding) {
  const Point &ssize(this->GetVirtualScreenSize());
  return Point( ssize.x - padding - size.x/2, padding + size.y/2 );
}



// TODO: move to different file

void GfxView::Look(const Vector3 &pos, const Vector3 &forward, float fovY, const Vector3 &up) {
  float aspect = (float)gfx.viewportSize.x / (float)gfx.viewportSize.y;

  this->right = forward.Cross(up);
  
  if (fovY > 0.0) {
    this->projStack.back() = Matrix4::Perspective(fovY, aspect, 0.0015f, 64.0f);
  } else {
    this->projStack.back() = Matrix4::Ortho(fovY*aspect, -fovY*aspect, fovY, -fovY, 0.0015f, 64.0f);
  }
  
  this->viewStack.back() = 
  this->modelViewStack.back() = Matrix4::LookFrom(pos, forward, up);
  glEnable(GL_DEPTH_TEST);
}

void GfxView::GUI() {
  this->projStack.back() = Matrix4();

  this->modelViewStack.back() = 
    Matrix4::Scale(    Vector3(2.0/gfx.screenSize.x, -2.0/gfx.screenSize.y, 1)) *
    Matrix4::Translate(Vector3( -gfx.screenSize.x/2, -gfx.screenSize.y/2,   0));
    
  if (gfx.screenSize.x > 640) {
    this->modelViewStack.back() = this->modelViewStack.back() * Matrix4::Scale(Vector3(2,2,1));
  }
  this->viewStack.back() = this->modelViewStack.back();
  glDisable(GL_DEPTH_TEST);
}

void GfxView::Push() {
  this->projStack.push_back(this->projStack.back());
  this->modelViewStack.push_back(this->modelViewStack.back());
  this->viewStack.push_back(this->viewStack.back());
  this->textureStack.push_back(this->textureStack.back());
}

void GfxView::Pop() {
  this->projStack.pop_back();
  this->modelViewStack.pop_back();
  this->viewStack.pop_back();
  this->textureStack.pop_back();

  if (projStack.empty())      projStack.push_back(Matrix4());
  if (modelViewStack.empty()) modelViewStack.push_back(Matrix4());
  if (viewStack.empty()) viewStack.push_back(Matrix4());
  if (textureStack.empty())   textureStack.push_back(Matrix4());
}

void GfxView::Translate(const Vector3 &p) {
  this->modelViewStack.back() = this->modelViewStack.back() * Matrix4::Translate(p);
}

void GfxView::Scale(const Vector3 &p) {
  this->modelViewStack.back() = this->modelViewStack.back() * Matrix4::Scale(p);
}

void GfxView::Rotate(float angle, const Vector3 &p)  {
  this->modelViewStack.back() = this->modelViewStack.back() * Matrix4::Rotate(angle, p);
}
  
void GfxView::Billboard(bool flip, bool vertical) {
  this->modelViewStack.back()(0,0) = flip?-1:1; 
  this->modelViewStack.back()(0,1) = 0;
  this->modelViewStack.back()(0,2) = 0;

  if (!vertical) {
    this->modelViewStack.back()(1,0) = 0; 
    this->modelViewStack.back()(1,1) = 1;
    this->modelViewStack.back()(1,2) = 0;
  }
    
  this->modelViewStack.back()(2,0) = 0; 
  this->modelViewStack.back()(2,1) = 0;
  this->modelViewStack.back()(2,2) = 1;
}

void GfxView::SetUniforms(const std::shared_ptr<Shader> &shader) const {
  shader->Uniform("u_matProjection",    this->projStack.back());
  shader->Uniform("u_matModelView",     this->modelViewStack.back());
  shader->Uniform("u_matView",          this->viewStack.back());
  shader->Uniform("u_matInvModelView",  this->modelViewStack.back().Inverse());
  shader->Uniform("u_matTexture",       this->textureStack.back());
  shader->Uniform("u_matNormal",        this->modelViewStack.back().Mat3().Inverse().Transpose());
}

bool GfxView::IsPointVisible(const Vector3 &p) const {
  Matrix4 matProjView(this->projStack.back() * this->viewStack.back());
  Vector3 p2 = matProjView*p;
  return p2.x > -1 && p2.x < 1 && p2.y > -1 && p2.y < 1 && p2.z > -0.001;
}

