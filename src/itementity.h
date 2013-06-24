#ifndef BARFOOS_ITEMENTITY_H
#define BARFOOS_ITEMENTITY_H

#include "common.h"

#include "mob.h"

class Item;

class ItemEntity : public Mob {
public:

  ItemEntity(const std::shared_ptr<Item> &item);
  virtual ~ItemEntity();

  virtual void Draw();
  virtual void OnUse(Entity *other);

protected:

  std::shared_ptr<Item> item;
};

#endif
