#include "item.h"
#include "entity.h"
#include "world.h"
#include "mob.h"

#include <GL/glfw.h>

extern int screenWidth, screenHeight;

Item::Item() {
  range = 5;
  cooldown = 1;
  nextUseT = 0;
}

Item::~Item() {
}

void Item::Use(Entity &entity, const Vector3 &pos, const Vector3 &dir, bool left) {
  if (glfwGetTime() < nextUseT) return;
  
  World *world = entity.GetWorld();
  
  AABB aabb;
  aabb.center = pos;
  aabb.extents = Vector3(range,range,range); 
  float dist = range;

  std::shared_ptr<Entity> selectedMob;
  Cell *selectedCell = nullptr;
  Vector3 p;
  float t;
 
  auto mobs = world->FindMobs(aabb);
  for (auto m : mobs) {
    if (m.get() == &entity) continue;

    if (m->GetAABB().Ray(pos, dir, t, p)) {
      if (t < dist) { 
        dist = t;
        selectedMob = m;
      }
    }
  }
  
  Side side;
  Cell &cell = world->CastRayCell(pos, dir, t, side);
  if (t < dist) {
    dist = t;
    selectedCell = &cell;
    selectedMob = nullptr;
  }

  if (selectedMob) {
    UseOnEntity(selectedMob, p, left);
  } else if (selectedCell) {
    UseOnCell(selectedCell, side, left);
  }
  nextUseT = cooldown + glfwGetTime();
}

void
Item::DrawIcon(float x, float y, float w, float h) {
  drawIcon(x,y, w, h, icon);
}

void
Item::DrawBillboard(const Vector3 &pos) {
  drawBillboard(pos+Vector3(0,0.25,0), 0.25, 0.25, icon);
}
