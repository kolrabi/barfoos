#include "GLee.h"
#include <GLFW/glfw3.h>
#include <GL/glu.h>

#include "gfx.h"
#include "game.h"
#include "vertex.h"
#include "shader.h"
#include "input.h"

Gfx::Gfx(const Point &pos, const Point &size, bool fullscreen) 
  : isInit(false),
    startTime(glfwGetTime()),
    screenPos(pos),
    screenSize(size),
    isFullscreen(fullscreen),
    mouseGrab(false),
    guiActiveCount(0)
{
  // unit cube vertices
  this->cubeVerts.push_back(Vertex(Vector3(-1, 1, -1), IColor(255,255,255), 0,0, Vector3( 0, 0,-1)));
  this->cubeVerts.push_back(Vertex(Vector3( 1, 1, -1), IColor(255,255,255), 1,0, Vector3( 0, 0,-1)));
  this->cubeVerts.push_back(Vertex(Vector3( 1,-1, -1), IColor(255,255,255), 1,1, Vector3( 0, 0,-1)));
  this->cubeVerts.push_back(Vertex(Vector3(-1,-1, -1), IColor(255,255,255), 0,1, Vector3( 0, 0,-1)));

  this->cubeVerts.push_back(Vertex(Vector3(-1,-1,  1), IColor(255,255,255), 0,1, Vector3( 0, 0, 1)));
  this->cubeVerts.push_back(Vertex(Vector3( 1,-1,  1), IColor(255,255,255), 1,1, Vector3( 0, 0, 1)));
  this->cubeVerts.push_back(Vertex(Vector3( 1, 1,  1), IColor(255,255,255), 1,0, Vector3( 0, 0, 1)));
  this->cubeVerts.push_back(Vertex(Vector3(-1, 1,  1), IColor(255,255,255), 0,0, Vector3( 0, 0, 1)));

  this->cubeVerts.push_back(Vertex(Vector3(-1, 1,  1), IColor(255,255,255), 0,1, Vector3( 0, 1, 0)));
  this->cubeVerts.push_back(Vertex(Vector3( 1, 1,  1), IColor(255,255,255), 1,1, Vector3( 0, 1, 0)));
  this->cubeVerts.push_back(Vertex(Vector3( 1, 1, -1), IColor(255,255,255), 1,0, Vector3( 0, 1, 0)));
  this->cubeVerts.push_back(Vertex(Vector3(-1, 1, -1), IColor(255,255,255), 0,0, Vector3( 0, 1, 0)));

  this->cubeVerts.push_back(Vertex(Vector3(-1,-1, -1), IColor(255,255,255), 0,0, Vector3( 0,-1, 0)));
  this->cubeVerts.push_back(Vertex(Vector3( 1,-1, -1), IColor(255,255,255), 1,0, Vector3( 0,-1, 0)));
  this->cubeVerts.push_back(Vertex(Vector3( 1,-1,  1), IColor(255,255,255), 1,1, Vector3( 0,-1, 0)));
  this->cubeVerts.push_back(Vertex(Vector3(-1,-1,  1), IColor(255,255,255), 0,1, Vector3( 0,-1, 0)));

  this->cubeVerts.push_back(Vertex(Vector3(-1, 1, -1), IColor(255,255,255), 0,0, Vector3(-1, 0, 0)));
  this->cubeVerts.push_back(Vertex(Vector3(-1, 1,  1), IColor(255,255,255), 1,0, Vector3(-1, 0, 0)));
  this->cubeVerts.push_back(Vertex(Vector3(-1,-1,  1), IColor(255,255,255), 1,1, Vector3(-1, 0, 0)));
  this->cubeVerts.push_back(Vertex(Vector3(-1,-1, -1), IColor(255,255,255), 0,1, Vector3(-1, 0, 0)));

  this->cubeVerts.push_back(Vertex(Vector3( 1,-1, -1), IColor(255,255,255), 0,1, Vector3( 1, 0, 0)));
  this->cubeVerts.push_back(Vertex(Vector3( 1,-1,  1), IColor(255,255,255), 1,1, Vector3( 1, 0, 0)));
  this->cubeVerts.push_back(Vertex(Vector3( 1, 1,  1), IColor(255,255,255), 1,0, Vector3( 1, 0, 0)));
  this->cubeVerts.push_back(Vertex(Vector3( 1, 1, -1), IColor(255,255,255), 0,0, Vector3( 1, 0, 0)));
  
  // quad
  this->quadVerts.push_back(Vertex(Vector3(-1,-1,  0), IColor(255,255,255), 0,0, Vector3( 0, 0, 1)));
  this->quadVerts.push_back(Vertex(Vector3( 1,-1,  0), IColor(255,255,255), 1,0, Vector3( 0, 0, 1)));
  this->quadVerts.push_back(Vertex(Vector3( 1, 1,  0), IColor(255,255,255), 1,1, Vector3( 0, 0, 1)));
  this->quadVerts.push_back(Vertex(Vector3(-1, 1,  0), IColor(255,255,255), 0,1, Vector3( 0, 0, 1)));
}

Gfx::~Gfx() {
  if (isInit) {
    this->Deinit();
  }
}

bool 
Gfx::Init() {
  // Set up glfw
  if (!glfwInit()) {
    std::cerr << "Could not initialize GLFW\n";
    return false;
  }

  // Create window
  this->window = glfwCreateWindow(screenSize.x, screenSize.y, "foobar", NULL, NULL);
  if (!this->window) {
    std::cerr << "Could not create window\n";
    glfwTerminate();
    return false;
  }
  
  if (this->screenPos.x != -1 || this->screenPos.y != -1)
    glfwSetWindowPos(this->window, this->screenPos.x, this->screenPos.y);
    
  glfwMakeContextCurrent(this->window);
  
  glfwSetWindowUserPointer(this->window, this);
  
  this->OnResize(this->screenSize);
  this->Viewport(Rect(Point(), this->screenSize));

  new Input();
  
  // Setup event handlers
  glfwSetWindowSizeCallback( this->window, [](GLFWwindow *window, int w, int h) { 
    Gfx *gfx = (Gfx*)glfwGetWindowUserPointer(window);
    gfx->OnResize(Point(w, h));   
  } );
  
  glfwSetCursorPosCallback(  this->window, [](GLFWwindow *window, double x, double y) { 
    Gfx *gfx = (Gfx*)glfwGetWindowUserPointer(window);
    gfx->OnMouseMove(Point(x,y)); 
  } );
  
  glfwSetMouseButtonCallback(this->window, [](GLFWwindow *window, int b, int e, int) { 
    Gfx *gfx = (Gfx*)glfwGetWindowUserPointer(window);
    gfx->OnMouseButton(b, e);
  } );
  
  glfwSetKeyCallback(        this->window, [](GLFWwindow *window, int k, int, int e, int) { 
    Gfx *gfx = (Gfx*)glfwGetWindowUserPointer(window);
    gfx->OnKey(k, e == GLFW_PRESS);
    Input::Instance->HandleKeyEvent(k, e != GLFW_RELEASE);
  } );
  
  //
  glfwSwapInterval(1);

  // We'd like extensions with that
  GLeeInit();
  
  // Basic GL settings
  glCullFace(GL_BACK);
  //glEnable(GL_CULL_FACE);
  
  glEnable(GL_ALPHA_TEST);
  glAlphaFunc(GL_GREATER, 0);
  
  glEnable(GL_TEXTURE_2D);
  glEnable(GL_SCISSOR_TEST);
  
  //glEnable(GL_DEPTH_TEST);
  //glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  // Colors look nicer unclamped
  if (GLEE_ARB_color_buffer_float) {
    glClampColorARB(GL_CLAMP_VERTEX_COLOR_ARB, GL_FALSE);
    glClampColorARB(GL_CLAMP_FRAGMENT_COLOR_ARB, GL_FALSE);
  }

  glHint(GL_FOG_HINT, GL_NICEST);

  this->noiseTex = noiseTexture(Point(256,256), Vector3(32,32,32));
  SetTextureFrame(this->noiseTex, 1);
  
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
  glfwTerminate();
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

bool
Gfx::Swap() {
  glfwSwapBuffers(this->window);
  glfwPollEvents();
  glViewport(0, 0, this->screenSize.x, this->screenSize.y);
  
  if (mouseGrab && !guiActiveCount && (mouseDelta.x || mouseDelta.y)) {
    glfwSetCursorPos(this->window, screenSize.x/2, screenSize.y/2);
    if (Game::Instance) Game::Instance->OnMouseDelta(mouseDelta);
  }
  mouseDelta = Point();

  updateTextures();
  
  if (Input::Instance->IsKeyActive(InputKey::DebugWireframe)) {
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  } else {
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  }
  
  return !glfwWindowShouldClose(this->window);
}

void Gfx::Viewport(const Rect &view) {
  if (view.size.x == 0 || view.size.y == 0) {
    glScissor(0,0, this->screenSize.x, this->screenSize.y);
    glViewport(0,0, this->screenSize.x, this->screenSize.y);
    this->viewportSize = this->screenSize;
  } else {
    glScissor(view.pos.x, view.pos.y, view.size.x, view.size.y);
    glViewport(view.pos.x, view.pos.y, view.size.x, view.size.y);
    this->viewportSize = view.size;
  }
}

void Gfx::View3D(const Vector3 &pos, const Vector3 &forward, float fovY, const Vector3 &up) const {
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  
  float aspect = (float)this->viewportSize.x / (float)this->viewportSize.y;
  if (fovY > 0.0) {
    gluPerspective(fovY, aspect, 0.0015f, 64.0f);
  } else {
    glOrtho(fovY*aspect, -fovY*aspect, fovY, -fovY, 0.0015f, 64.0f);
  }
  
  Vector3 tpos = pos + forward;
  
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  gluLookAt(pos.x, pos.y, pos.z, tpos.x, tpos.y, tpos.z, up.x, up.y, up.z);
  glEnable(GL_DEPTH_TEST);
}

void Gfx::ViewGUI() const {
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  
  glScalef(2.0/this->screenSize.x, -2.0/this->screenSize.y, 1);
  glTranslatef(-this->screenSize.x/2,-this->screenSize.y/2, 0);
  if (this->screenSize.x > 640) {
    glScalef(2,2,1);
  }
  glDisable(GL_DEPTH_TEST);
}

void Gfx::ViewPush() const {
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
}

void Gfx::ViewPop() const {
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
}

void Gfx::ViewTranslate(const Vector3 &p) const {
  glMatrixMode(GL_MODELVIEW);
  glTranslatef(p.x, p.y, p.z);
}

void Gfx::ViewScale(const Vector3 &p) const {
  glMatrixMode(GL_MODELVIEW);
  glScalef(p.x, p.y, p.z);
}

void Gfx::ViewRotate(float angle, const Vector3 &p) const {
  glMatrixMode(GL_MODELVIEW);
  glRotatef(angle, p.x, p.y, p.z);
}

void Gfx::SetDepthTest(bool on) const {
  if (on) {
    //glEnable(GL_DEPTH_TEST);
  } else {
    glDisable(GL_DEPTH_TEST);
  }
}

void Gfx::SetCullFace(bool on) const {
  if (on) {
//    glEnable(GL_CULL_FACE);
  } else {
    glDisable(GL_CULL_FACE);
  }
}

void Gfx::SetShader(const Shader *shader) const {
  if (!shader) {
    glUseProgramObjectARB(0);
    return;
  }

  glUseProgramObjectARB(shader->GetProgram());
  shader->Uniform("u_fogExp2",  this->fogExp2);
  shader->Uniform("u_fogLin",   this->fogLin);
  shader->Uniform("u_fogColor", this->fogColor);
  shader->Uniform("u_time",     this->GetTime());
}

void 
Gfx::SetFog(float e, float l, const IColor &color) {
  this->fogExp2 = e;
  this->fogLin = l;
  this->fogColor = color;
/*
  // Light fog for the right mood
  float c[4] = { color.r/255.0, color.b/255.0, 0,1 };
  glEnable(GL_FOG);
  glFogfv(GL_FOG_COLOR, black);
  glFogf(GL_FOG_START, 0);
  glFogf(GL_FOG_END, 64);
  
  glFogi(GL_FOG_MODE, GL_EXP2);
  glFogf(GL_FOG_DENSITY, e);
  */
}


void 
Gfx::SetTextureFrame(const Texture *texture, size_t stage, size_t currentFrame, size_t frameCount) const {
  glActiveTexture(GL_TEXTURE0 + stage);
  glEnable(GL_TEXTURE_2D);
  
  glMatrixMode(GL_TEXTURE);
  glLoadIdentity();
  
  if (frameCount > 1) {
    glScalef(1.0/frameCount, 1, 1);
    glTranslatef(currentFrame, 0, 0);
  }
  
  glMatrixMode(GL_MODELVIEW);
  
  if (texture) {
    glBindTexture(GL_TEXTURE_2D, texture->handle);
    //std::cerr << "binding " << texture->handle << std::endl;
  } else {
    glBindTexture(GL_TEXTURE_2D, 0);
  }
}

void
Gfx::SetColor(const IColor &color) const {
  glColor3f(color.r/255.0, color.g/255.0, color.b/255.0);
}

void 
Gfx::DrawTriangles(const std::vector<Vertex> &vertices) const {
  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_COLOR_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  
  glInterleavedArrays(GL_T2F_C4F_N3F_V3F, sizeof(Vertex), &vertices[0]);

  glDrawArrays(GL_TRIANGLES, 0, vertices.size());

  glDisableClientState(GL_TEXTURE_COORD_ARRAY);
  glDisableClientState(GL_COLOR_ARRAY);
  glDisableClientState(GL_VERTEX_ARRAY);
}

void 
Gfx::DrawQuads(const std::vector<Vertex> &vertices) const {
  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_COLOR_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  
  glInterleavedArrays(GL_T2F_C4F_N3F_V3F, sizeof(Vertex), &vertices[0]);

  glDrawArrays(GL_QUADS, 0, vertices.size());

  glDisableClientState(GL_TEXTURE_COORD_ARRAY);
  glDisableClientState(GL_COLOR_ARRAY);
  glDisableClientState(GL_VERTEX_ARRAY);
}

void Gfx::DrawUnitCube() const {
  this->DrawQuads(this->cubeVerts);
}    

void Gfx::DrawAABB(const AABB &aabb) const {
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  
  glTranslatef(aabb.center.x, aabb.center.y, aabb.center.z);
  glScalef(aabb.extents.x, aabb.extents.y, aabb.extents.z);
  
  this->DrawUnitCube();
  
  glPopMatrix();
}

void Gfx::DrawSprite(const Sprite &sprite, const Vector3 &pos, bool billboard) const {
  this->SetTextureFrame(sprite.texture, 0, sprite.currentFrame, sprite.totalFrames);

  this->ViewPush();
  this->ViewTranslate(pos);

  if (billboard) {
    float m[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, m);
    
    m[0] = 1; m[1] = 0; m[2]  = 0;
    if (!sprite.vertical) { m[4] = 0; m[5] = 1; m[6]  = 0; }
    m[8] = 0; m[9] = 0; m[10] = 1;
    
    glLoadMatrixf(m);
    this->ViewTranslate(Vector3(sprite.offsetX, sprite.offsetY, 0));
  }
  
  this->ViewScale(Vector3(sprite.width/2, sprite.height/2, 1));
 
  this->DrawQuads(this->quadVerts);

  this->ViewPop();
}

void Gfx::DrawIcon(const Sprite &sprite, const Point &center, const Point &size) const {
  this->SetTextureFrame(sprite.texture, 0, sprite.currentFrame, sprite.totalFrames);

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  
  glTranslatef(center.x, center.y, 0);
  glTranslatef(sprite.offsetX*size.x, sprite.offsetY*size.y, 0);
  glScalef((sprite.width*size.x)/2, -(sprite.height*size.y)/2, 1);
 
  this->DrawQuads(this->quadVerts);

  glPopMatrix();
}

void 
Gfx::OnResize(const Point &size) {
  // update screen size
  this->screenSize = size;
  if (size.x <= 640) {
    this->virtualScreenSize.x = size.x;
    this->virtualScreenSize.y = size.y;
  } else {
    this->virtualScreenSize.x = size.x/2;
    this->virtualScreenSize.y = size.y/2;
  }
}

void 
Gfx::OnMouseMove(const Point &pos) {
  if (guiActiveCount) {
    //mouseDelta = Point();
    mousePos.x = (pos.x / (float)screenSize.x)*virtualScreenSize.x;
    mousePos.y = (pos.y / (float)screenSize.y)*virtualScreenSize.y;
    
    if (Game::Instance) Game::Instance->OnMouseMove(mousePos);
  } else if (mouseGrab) {
    mouseDelta.x = pos.x-screenSize.x/2;
    mouseDelta.y = pos.y-screenSize.y/2;
  }
}

void 
Gfx::OnMouseButton(int button, int event) {
  bool down = event == GLFW_PRESS;
  
  if (!mouseGrab && down && button == GLFW_MOUSE_BUTTON_LEFT) {
    if (!guiActiveCount) {
      glfwSetCursorPos(this->window, screenSize.x/2, screenSize.y/2);
      glfwSetInputMode(this->window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    }
    mouseGrab = true;
  } else {
    if (Game::Instance) Game::Instance->OnMouseClick(mousePos, button, down);
  }
}

void 
Gfx::OnKey(int key, int event) {
  bool down = event == GLFW_PRESS;
  
  if (mouseGrab && down && key == GLFW_KEY_ESCAPE) {
    glfwSetInputMode(this->window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    mouseGrab = false;
  }
}

Point alignBottomLeftScreen(const Point &size, int padding) {
  const Point &ssize(Game::Instance->GetGfx()->GetVirtualScreenSize());
  return Point( padding + size.x/2, ssize.y - padding - size.y/2 );
}

Point alignBottomRightScreen(const Point &size, int padding) {
  const Point &ssize(Game::Instance->GetGfx()->GetVirtualScreenSize());
  return Point( ssize.x - padding - size.x/2, ssize.y - padding - size.y/2 );
}

Point alignTopLeftScreen(const Point &size, int padding) {
  return Point( padding + size.x/2, padding + size.y/2 );
}

Point alignTopRightScreen(const Point &size, int padding) {
  const Point &ssize(Game::Instance->GetGfx()->GetVirtualScreenSize());
  return Point( ssize.x - padding - size.x/2, padding + size.y/2 );
}
