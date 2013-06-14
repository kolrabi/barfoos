#ifndef BARFOOS_ITEM_H
#define BARFOOS_ITEM_H

#include "common.h"

class World;
class Mob;
class Entity;
class Cell;

struct ItemClass {
  std::string name;
};

class Item {
public:

  Item();
  virtual ~Item();

  virtual void Use(Entity &entity, const Vector3 &pos, const Vector3 &dir, bool left);

  virtual void UseOnEntity(const std::shared_ptr<Entity> &ent, const Vector3 &p, bool left) = 0;
  virtual void UseOnCell(Cell *cell, Side side, bool left) = 0;
  virtual void Draw() {}
  virtual void DrawIcon(float x, float y);

protected:

  float range;  
  unsigned int icon;
  
  float cooldown;
  float nextUseT;
};

#endif

