#include "weapon.h"
#include "world.h"
#include "entity.h"

#include <GL/glfw.h>

Weapon::Weapon() {
  this->texture = loadTexture("items/sword2");
  this->cooldown = 0.2;
}

Weapon::~Weapon() {
}

void Weapon::UseOnEntity(const std::shared_ptr<Entity> &mob, const Vector3 &p, bool left) {
  (void)p;
  if (left) mob->AddHealth(-1);
}

void Weapon::UseOnCell(Cell *cell, Side side, bool left) {
  if (left) {
    cell->GetWorld()->SetCell(cell->GetPosition(), Cell("air"));
  } else {
    cell->GetWorld()->SetCell(cell->GetPosition()[side], Cell("lava"));
  }
}

void Weapon::Draw() {
  float now = glfwGetTime();
  
  glDisable(GL_CULL_FACE);
  glScalef(1, 1, 1);
  glTranslatef(-1, -2, 4);
  // glScalef(0.25, 0.25, 0.5);
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
}
