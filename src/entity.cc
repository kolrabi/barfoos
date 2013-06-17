#include "entity.h"
#include "world.h"
#include "cell.h"
#include "util.h"

#include <GL/glfw.h>

Entity::Entity() {
  lastT = 0;
  deltaT = 0;
  
  frame = 0;
  frames = 1;
  animation = 0;
  anims.push_back(Animation(0,1,10));
  
  aabb.extents = Vector3(0.5,0.5,0.5);

  texture = 0;
  aboveTexture = 0;
  belowTexture = 0;

  inventory.resize(48, nullptr);
}

Entity::~Entity() {
}

void 
Entity::Update(float t) {
  if (!this->world) return;
  
  // update time
  if (lastT == 0) lastT = t;
  deltaT = t - lastT;
  
  this->light = this->world->GetLight(IVector3(aabb.center.x, aabb.center.y, aabb.center.z)).Saturate();
}

void
Entity::Draw() {
  if (this->texture == 0) return;

  float u = 0.0;
  float uw = 1.0;
  if (frames) {
    int f = ((int)frame) % frames;
    uw = 1.0/frames;
    u = f*uw;
  }
  
  glColor3ub(light.r, light.g, light.b);
  drawBillboard(aabb.center, aabb.extents.x, aabb.extents.y, texture, u, uw);
  
  if (glfwGetKey('E')) {
    glColor3f(0.25,0.25,0.25);
    this->DrawBoundingBox();
  }
}

void
Entity::DrawBoundingBox() {
  glPushMatrix();
  glTranslatef(aabb.center.x, aabb.center.y, aabb.center.z);
  glScalef(aabb.extents.x, aabb.extents.y, aabb.extents.z);
  glDisable(GL_CULL_FACE);
    
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE);
  glDepthMask(GL_FALSE);
    
  glBindTexture(GL_TEXTURE_2D, 0);
  drawUnitCube();
  glDepthMask(GL_TRUE);
  glDisable(GL_BLEND);
  glEnable(GL_CULL_FACE);
  glPopMatrix();
}

void
Entity::AddHealth(int points) {
  if (health == 0) return;
  
  health += points;
  std::cerr << health << std::endl;
  if (health <= 0) {
    health = 0;
    Die();
  }
}

