#include "weapon.h"
#include "world.h"
#include "entity.h"

#include <GL/glfw.h>

Weapon::Weapon() {
  this->texture = loadTexture("items/sword2");
  this->icon = loadTexture("items/sword2");
  this->cooldown = 0.2;
}

Weapon::~Weapon() {
}

void Weapon::UseOnEntity(const std::shared_ptr<Entity> &entity) {
  if (!this->CanUse()) return;
  
  entity->AddHealth(-1);
  
  this->StartCooldown();
}

void Weapon::UseOnCell(Cell *cell, Side side) {
  (void)side;
  
  cell->GetWorld()->SetCell(cell->GetPosition(), Cell("air"));
}

void Weapon::Draw(bool left) {
  float now = glfwGetTime();

  glPushMatrix();
  
  glDisable(GL_CULL_FACE);
  glScalef(left ? -1 : 1, 1, 1);
  glTranslatef(-1, -2, 4);
  glRotatef(-60, 0, 1, 0);

  // cooldown fraction: 1.0 full cooldown left, 0.0 cooldown over
  float f = (nextUseT - now)/cooldown;
  if (f < 0) f = 0;

  glTranslatef(-1,-1,0);
  glRotatef(60-f*60, 0,0,1);
  glTranslatef(1,1,0);
  
  glBindTexture(GL_TEXTURE_2D, texture);
  glBegin(GL_QUADS);
  glTexCoord2f(1,1); glVertex3f(-1, 1,0);
  glTexCoord2f(0,1); glVertex3f( 1, 1,0);
  glTexCoord2f(0,0); glVertex3f( 1,-1,0);
  glTexCoord2f(1,0); glVertex3f(-1,-1,0);
  glEnd();
  
  glPopMatrix();
}
