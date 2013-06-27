#include "GLee.h"
#include <GL/glfw.h>

#include "gfx.h"
#include "game.h"

std::unique_ptr<Gfx> Gfx::Instance = nullptr;

Gfx::Gfx(const Point &pos, const Point &size, bool fullscreen) 
  : isInit(false),
    startTime(glfwGetTime()),
    screenPos(pos),
    screenSize(size),
    isFullscreen(fullscreen),
    mouseGrab(false),
    guiActiveCount(0)
{
  Gfx::Instance = std::unique_ptr<Gfx>(this);
  
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
    glfwCloseWindow();
    glfwTerminate();
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
  if (!glfwOpenWindow(screenSize.x, screenSize.y, 8, 8, 8, 0, 24, 0, GLFW_WINDOW)) {
    std::cerr << "Could not open window\n";
    return false;
  }
  
  if (this->screenPos.x != -1 || this->screenPos.y != -1)
    glfwSetWindowPos(this->screenPos.x, this->screenPos.y);
  
  this->OnResize(this->screenSize);
  this->Viewport(Rect(Point(), this->screenSize));
  
  // Setup event handlers
  glfwSetWindowSizeCallback( [](int w, int h) { Gfx::Instance->OnResize(Point(w, h));   } );
  glfwSetMousePosCallback(   [](int x, int y) { Gfx::Instance->OnMouseMove(Point(x,y)); } );
  glfwSetMouseButtonCallback([](int b, int e) { Gfx::Instance->OnMouseButton(b, e);     } );
  glfwSetKeyCallback(        [](int k, int e) { Gfx::Instance->OnKey(k, e);             } );
  
  //
  glfwSwapInterval(1);

  // We'd like extensions with that
  GLeeInit();
  
  // Basic GL settings
  glCullFace(GL_BACK);
  glEnable(GL_CULL_FACE);
  
  glEnable(GL_ALPHA_TEST);
  glAlphaFunc(GL_GREATER, 0);
  
  glEnable(GL_TEXTURE_2D);
  glEnable(GL_SCISSOR_TEST);
  
  glEnable(GL_DEPTH_TEST);

  // Colors look nicer unclamped
  if (GLEE_ARB_color_buffer_float) {
    glClampColorARB(GL_CLAMP_VERTEX_COLOR_ARB, GL_FALSE);
    glClampColorARB(GL_CLAMP_FRAGMENT_COLOR_ARB, GL_FALSE);
  }

  // Light fog for the right mood
  float black[4] = { 0,0,0,1 };
  glEnable(GL_FOG);
  glFogfv(GL_FOG_COLOR, black);
  glFogf(GL_FOG_START, 0);
  glFogf(GL_FOG_END, 64);
  glHint(GL_FOG_HINT, GL_NICEST);

  isInit = true;
  return true;
}

void
Gfx::Deinit() {
  Gfx::Instance = nullptr;
}

void 
Gfx::IncGuiCount() {
  guiActiveCount ++;
  glfwEnable(GLFW_MOUSE_CURSOR);
}

void
Gfx::DecGuiCount() {
  if (!guiActiveCount) return;
  
  guiActiveCount--;
  
  if (!guiActiveCount && mouseGrab) {
    glfwSetMousePos(screenSize.x/2, screenSize.y/2);
    // glfwDisable(GLFW_MOUSE_CURSOR);  
  }
}

bool
Gfx::Swap() {
  glfwSwapBuffers();
  glViewport(0, 0, this->screenSize.x, this->screenSize.y);
  
  updateTextures();
  mouseDelta = Point();
  return glfwGetWindowParam(GLFW_OPENED);
}

float
Gfx::GetTime() const {
  return glfwGetTime() - startTime;
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

void 
Gfx::SetTextureFrame(size_t currentFrame, size_t frameCount) const {
  glMatrixMode(GL_TEXTURE);
  glLoadIdentity();
  glScalef(1.0/frameCount, 1, 1);
  glTranslatef(currentFrame, 0, 0);
  glMatrixMode(GL_MODELVIEW);
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

void Gfx::DrawUnitCube(unsigned int tex) const {
  glBindTexture(GL_TEXTURE_2D, tex);
  this->DrawQuads(this->cubeVerts);
}    

void Gfx::DrawAABB(const AABB &aabb, unsigned int tex) const {
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  
  glTranslatef(aabb.center.x, aabb.center.y, aabb.center.z);
  glScalef(aabb.extents.x, aabb.extents.y, aabb.extents.z);
  
  this->DrawUnitCube(tex);
  
  glPopMatrix();
}

void Gfx::DrawSprite(const Sprite &sprite, const Vector3 &pos, bool billboard) const {
  this->SetTextureFrame(sprite.currentFrame, sprite.totalFrames);

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  
  glTranslatef(pos.x, pos.y, pos.z);

  if (billboard) {
    float m[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, m);
    
    m[0] = 1; m[1] = 0; m[2]  = 0;
    if (!sprite.vertical) m[4] = 0; m[5] = 1; m[6]  = 0;
    m[8] = 0; m[9] = 0; m[10] = 1;
    
    glLoadMatrixf(m);
    glTranslatef(sprite.offsetX, sprite.offsetY, 0);
  }
  
  glScalef(sprite.width/2, sprite.height/2, 1);
 
  glBindTexture(GL_TEXTURE_2D, sprite.texture);

  this->DrawQuads(this->quadVerts);

  glPopMatrix();
  
  this->SetTextureFrame(0, 1);
}

void Gfx::DrawIcon(const Sprite &sprite, const Point &center, const Point &size) const {
  this->SetTextureFrame(sprite.currentFrame, sprite.totalFrames);

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  
  glTranslatef(center.x, center.y, 0);
  glTranslatef(sprite.offsetX*size.x, sprite.offsetY*size.y, 0);
  glScalef((sprite.width*size.x)/2, -(sprite.height*size.y)/2, 1);
 
  glBindTexture(GL_TEXTURE_2D, sprite.texture);

  this->DrawQuads(this->quadVerts);

  glPopMatrix();
  
  this->SetTextureFrame(0, 1);
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
    mouseDelta = Point();
    mousePos.x = (pos.x / (float)screenSize.x)*virtualScreenSize.x;
    mousePos.y = (pos.y / (float)screenSize.y)*virtualScreenSize.y;
    
    if (Game::Instance) Game::Instance->OnMouseMove(mousePos);
  } else if (mouseGrab) {
    mouseDelta.x = pos.x-screenSize.x/2;
    mouseDelta.y = pos.y-screenSize.y/2;
    glfwSetMousePos(screenSize.x/2, screenSize.y/2);
    
    if (Game::Instance) Game::Instance->OnMouseDelta(mouseDelta);
  }
}

void 
Gfx::OnMouseButton(int button, int event) {
  bool down = event == GLFW_PRESS;
  
  if (!mouseGrab && down && button == GLFW_MOUSE_BUTTON_LEFT) {
    if (!guiActiveCount) {
      glfwSetMousePos(screenSize.x/2, screenSize.y/2);
      // glfwDisable(GLFW_MOUSE_CURSOR);
    }
    mouseGrab = true;
  } else {
    if (Game::Instance) Game::Instance->OnMouseClick(mousePos, button, down);
  }
}

void 
Gfx::OnKey(int key, int event) {
  bool down = event == GLFW_PRESS;
  
  if (mouseGrab && down && glfwGetKey(GLFW_KEY_ESC)) {
    glfwEnable(GLFW_MOUSE_CURSOR);
    mouseGrab = false;
  }

  if (Game::Instance) Game::Instance->OnKey(key, down);
}

Point alignBottomLeftScreen(const Point &size, int padding) {
  const Point &ssize(Gfx::Instance->GetVirtualScreenSize());
  return Point( padding + size.x/2, ssize.y - padding - size.y/2 );
}

Point alignBottomRightScreen(const Point &size, int padding) {
  const Point &ssize(Gfx::Instance->GetVirtualScreenSize());
  return Point( ssize.x - padding - size.x/2, ssize.y - padding - size.y/2 );
}

Point alignTopLeftScreen(const Point &size, int padding) {
  return Point( padding + size.x/2, padding + size.y/2 );
}

Point alignTopRightScreen(const Point &size, int padding) {
  const Point &ssize(Gfx::Instance->GetVirtualScreenSize());
  return Point( ssize.x - padding - size.x/2, padding + size.y/2 );
}
