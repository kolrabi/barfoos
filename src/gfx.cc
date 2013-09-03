#include "common.h"

#include "gfx.h"
#include "gfxview.h"

#include "game.h"
#include "vertex.h"
#include "shader.h"
#include "input.h"
#include "texture.h"
#include "player.h"
#include "sprite.h"
#include "vertexbuffer.h"
#include "matrix4.h"

#include "GLee.h"
#include <GLFW/glfw3.h>

Gfx::Gfx(const Point &pos, const Point &size, bool fullscreen) :
  screen(*this, pos, size, fullscreen),
  useFixedFunction(false),
  isInit(false),
  startTime(glfwGetTime()),
  player(nullptr),

  vb(nullptr),
  noiseTex(nullptr),
  cubeVerts(0),
  quadVerts(0),

  activeShader(nullptr),
  view(new GfxView(*this)),
  color(255, 255, 255),
  light(0,0,0),
  alpha(1.0),
  activeTextures(),
  activeTextureStage(0),
  activeVertexPointer(nullptr),

  fogLin(0.05),
  fogColor(64, 64, 64),

  lightPositions(MaxLights),
  lightColors(MaxLights)
{
}

Gfx::~Gfx() {
  if (isInit) {
    this->Deinit();
  }
  delete this->view;
}

bool
Gfx::Init(Game &game) {
  Log("Initializing GFX\n");

  if (!this->screen.Init(game)) return false;

  // We'd like extensions with that
  GLeeInit();

  if (!GLEE_ARB_shader_objects) {
    Log("\n******************************************************************\n");
    Log("  Your OpenGL does not support the ARB_shader_objects extension!\n");
    Log("  Falling back to stone age rendering path!\n");
    Log("******************************************************************\n\n");
    this->useFixedFunction = true;
  }
  if (!GLEE_ARB_vertex_buffer_object) {
    Log("\n************************************************************************\n");
    Log("  Your OpenGL does not support the ARB_vertex_buffer_object extension!\n");
    Log("**********************************************************************\n\n");
    //this->useVBO = false;
  }

  // Basic GL settings
  glCullFace(GL_BACK);
  glEnable(GL_CULL_FACE);

  if (this->useFixedFunction) {
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0);
  }

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

  //this->noiseTex = Texture::Create("*noise", Image::Noise(Point(256,256), Vector3(32,32,32)));
  //SetTextureFrame(this->noiseTex, 1);

  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_COLOR_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);

  this->vb = new VertexBuffer();

  // quad
  this->vb->Add(Vertex(Vector3(-1,-1,  0), IColor(255,255,255), 0,0, Vector3( 0, 0, 1)));
  this->vb->Add(Vertex(Vector3( 1,-1,  0), IColor(255,255,255), 1,0, Vector3( 0, 0, 1)));
  this->vb->Add(Vertex(Vector3( 1, 1,  0), IColor(255,255,255), 1,1, Vector3( 0, 0, 1)));
  this->vb->Add(Vertex(Vector3(-1, 1,  0), IColor(255,255,255), 0,1, Vector3( 0, 0, 1)));

  // unit cube vertices
  this->vb->Add(Vertex(Vector3( 1, 1, -1), IColor(255,255,255), 0,1, Vector3( 1, 0, 0)));
  this->vb->Add(Vertex(Vector3( 1, 1,  1), IColor(255,255,255), 1,1, Vector3( 1, 0, 0)));
  this->vb->Add(Vertex(Vector3( 1,-1,  1), IColor(255,255,255), 1,0, Vector3( 1, 0, 0)));
  this->vb->Add(Vertex(Vector3( 1,-1, -1), IColor(255,255,255), 0,0, Vector3( 1, 0, 0)));

  this->vb->Add(Vertex(Vector3(-1,-1, -1), IColor(255,255,255), 1,0, Vector3(-1, 0, 0)));
  this->vb->Add(Vertex(Vector3(-1,-1,  1), IColor(255,255,255), 2,0, Vector3(-1, 0, 0)));
  this->vb->Add(Vertex(Vector3(-1, 1,  1), IColor(255,255,255), 2,1, Vector3(-1, 0, 0)));
  this->vb->Add(Vertex(Vector3(-1, 1, -1), IColor(255,255,255), 1,1, Vector3(-1, 0, 0)));

  this->vb->Add(Vertex(Vector3(-1,-1,  1), IColor(255,255,255), 2,0, Vector3( 0, 0, 1)));
  this->vb->Add(Vertex(Vector3( 1,-1,  1), IColor(255,255,255), 3,0, Vector3( 0, 0, 1)));
  this->vb->Add(Vertex(Vector3( 1, 1,  1), IColor(255,255,255), 3,1, Vector3( 0, 0, 1)));
  this->vb->Add(Vertex(Vector3(-1, 1,  1), IColor(255,255,255), 2,1, Vector3( 0, 0, 1)));

  this->vb->Add(Vertex(Vector3(-1, 1, -1), IColor(255,255,255), 3,1, Vector3( 0, 0,-1)));
  this->vb->Add(Vertex(Vector3( 1, 1, -1), IColor(255,255,255), 4,1, Vector3( 0, 0,-1)));
  this->vb->Add(Vertex(Vector3( 1,-1, -1), IColor(255,255,255), 4,0, Vector3( 0, 0,-1)));
  this->vb->Add(Vertex(Vector3(-1,-1, -1), IColor(255,255,255), 3,0, Vector3( 0, 0,-1)));

  this->vb->Add(Vertex(Vector3(-1, 1,  1), IColor(255,255,255), 4,0, Vector3( 0, 1, 0)));
  this->vb->Add(Vertex(Vector3( 1, 1,  1), IColor(255,255,255), 5,0, Vector3( 0, 1, 0)));
  this->vb->Add(Vertex(Vector3( 1, 1, -1), IColor(255,255,255), 5,1, Vector3( 0, 1, 0)));
  this->vb->Add(Vertex(Vector3(-1, 1, -1), IColor(255,255,255), 4,1, Vector3( 0, 1, 0)));

  this->vb->Add(Vertex(Vector3(-1,-1, -1), IColor(255,255,255), 5,1, Vector3( 0,-1, 0)));
  this->vb->Add(Vertex(Vector3( 1,-1, -1), IColor(255,255,255), 6,1, Vector3( 0,-1, 0)));
  this->vb->Add(Vertex(Vector3( 1,-1,  1), IColor(255,255,255), 6,0, Vector3( 0,-1, 0)));
  this->vb->Add(Vertex(Vector3(-1,-1,  1), IColor(255,255,255), 5,0, Vector3( 0,-1, 0)));

  isInit = true;
  return true;
}

void
Gfx::Deinit() {
  delete this->vb;

  this->screen.Deinit();
}

float
Gfx::GetTime() const {
  return glfwGetTime();
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
  this->screen.Update(game);

  Texture::UpdateTextures();

  if (game.GetInput().IsKeyActive(InputKey::DebugWireframe)) {
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  } else {
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  }
}

void Gfx::SetShader(const std::string &name) {
  if (this->useFixedFunction) return;

  if (name == "") {
    glUseProgramObjectARB(0);
    this->activeShader = nullptr;
    return;
  }

  if (!shaders[name]) shaders[name] = std::shared_ptr<Shader>(new Shader(name));
  this->activeShader = shaders[name];

  glUseProgramObjectARB(shaders[name]->GetProgram());
  this->SetUniforms();
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

  this->view->textureStack.back() = Matrix4();

  if (frameCount > 1) {
    this->view->textureStack.back() =
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
Gfx::SetLight(const IColor &color) {
  this->light = color;
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
  if (this->useFixedFunction) {
    // TODO: lights

    this->view->SetUniforms(nullptr);
    if (this->player) this->player->SetUniforms(nullptr);
    return;
  }

  if (!this->activeShader) return;

  std::vector<Vector3> lightPos;
  std::vector<IColor>  lightCol;

  for (size_t i=0; i<this->lightPositions.size(); i++) {
//    if (this->view->IsPointVisible(this->lightPositions[i])) {
      lightPos.push_back(this->lightPositions[i]);
      lightCol.push_back(this->lightColors[i]);
//    }
  }

  lightCol.resize(MaxLights, IColor(0,0,0));
  lightPos.resize(MaxLights);

  this->view->SetUniforms(this->activeShader);

  this->activeShader->Uniform("u_fogLin",   this->fogLin);
  this->activeShader->Uniform("u_fogColor", this->fogColor);
  this->activeShader->Uniform("u_time",     this->GetTime());
  this->activeShader->Uniform("u_color",    this->color, this->alpha);
  this->activeShader->Uniform("u_light",    this->light, 1.0);

  this->activeShader->Uniform("u_lightPos",   lightPos);
  this->activeShader->Uniform("u_lightColor", lightCol);

  this->activeShader->Uniform("u_texture", 0);
  this->activeShader->Uniform("u_texture2", 1);

  if (this->player) this->player->SetUniforms(this->activeShader);
}

void
Gfx::DrawTriangles(VertexBuffer &vb, size_t first, size_t count) {
  this->SetUniforms();
  vb.DrawTriangles(first, count);
}

void
Gfx::DrawQuads(VertexBuffer &vb, size_t first, size_t count) {
  this->SetUniforms();
  vb.DrawQuads(first, count);
}

void Gfx::DrawUnitCube() {
  this->DrawQuads(*this->vb, 4, 24);
}

void Gfx::DrawUnitQuad() {
  this->DrawQuads(*this->vb, 0, 4);
}

void Gfx::DrawAABB(const AABB &aabb) {
  this->view->Push();
  this->view->Translate(aabb.center);
  this->view->Scale(aabb.extents);
  this->DrawUnitCube();
  this->view->Pop();
}

void Gfx::DrawSprite(const Sprite &sprite, const Vector3 &pos, bool flip, bool billboard, float angleV) {
  this->view->Push();
  this->view->Translate(pos);

  if (billboard) {
    this->view->Billboard(flip, sprite.vertical);
    this->view->Translate(Vector3(sprite.offsetX, sprite.offsetY, 0));
  }
  this->view->Rotate(angleV, Vector3(0,1,0));
  this->view->Scale(Vector3(sprite.width/2, sprite.height/2, 1));

  SetBackfaceCulling(false);
  if (sprite.texture) {
    if (this->useFixedFunction) {
      glEnable(GL_LIGHTING);
      float e[] = {
        this->light.r / 255.0f, this->light.g / 255.0f, this->light.b / 255.0f, 1.0f
      };
      glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, e);
    }
    this->SetTextureFrame(sprite.texture, 0, sprite.currentFrame, sprite.totalFrames);
    this->DrawUnitQuad();
    if (this->useFixedFunction) {
      glDisable(GL_LIGHTING);
    }
  }
  if (sprite.emissiveTexture) {
    this->SetTextureFrame(sprite.emissiveTexture, 0, sprite.currentFrame, sprite.totalFrames);
    this->SetBlendAdd();
    this->SetColor(IColor(255,255,255));
    this->SetLight(IColor(255,255,255));
    this->DrawUnitQuad();
    this->SetBlendNormal();
  }
  SetBackfaceCulling(true);

  this->view->Pop();
}

void Gfx::DrawIcon(const Sprite &sprite, const Point &center, const Point &size) {
  this->view->Push();
  this->view->Translate(Vector3(center.x + sprite.offsetX*size.x, center.y + sprite.offsetY*size.y, 0));
  this->view->Scale    (Vector3(size.x/2, -size.y/2, 1));
  if (sprite.texture) {
    this->SetTextureFrame(sprite.texture, 0, sprite.currentFrame, sprite.totalFrames);
    this->DrawUnitQuad();
  }
  if (sprite.emissiveTexture) {
    this->SetTextureFrame(sprite.emissiveTexture, 0, sprite.currentFrame, sprite.totalFrames);
    this->SetBlendAdd();
    this->DrawUnitQuad();
    this->SetBlendNormal();
  }
  this->view->Pop();
}

void Gfx::DrawIconQuad(const Point &center, const Point &size) {
  this->view->Push();
  this->view->Translate(Vector3(center.x, center.y, 0));
  this->view->Scale    (Vector3(size.x/2, -size.y/2, 1));
  this->DrawUnitQuad();
  this->view->Pop();
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

  this->DrawUnitQuad();
}
