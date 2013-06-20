#include "item.h"
#include "entity.h"
#include "world.h"
#include "mob.h"

#include <GL/glfw.h>

extern int screenWidth, screenHeight;

Item::Item() {
  this->range = 5;
  this->equippable = 0;
  this->twoHanded = false;
  this->cooldown = 1;
  this->nextUseT = 0;
}

Item::~Item() {
}

bool Item::CanUse() const {
  return this->nextUseT < glfwGetTime();
}
  
void Item::StartCooldown() {
  this->nextUseT = glfwGetTime() + this->cooldown;
}

/*
void
Item::DrawBillboard(const Vector3 &pos) {
  drawBillboard(pos+Vector3(0,0.25,0), 0.25, 0.25, icon);
}
*/