#ifndef BARFOOS_ITEMENTITY_H
#define BARFOOS_ITEMENTITY_H

#include "common.h"

#include "mob.h"

class Item;

class ItemEntity : public Mob {
public:

  ItemEntity(const std::string &itemName);
  ItemEntity(const std::shared_ptr<Item> &item);
  virtual ~ItemEntity();

  virtual void Draw(Gfx &gfx) const override;
  virtual void Update(Game &game) override;
  
  virtual void OnUse(Game &game, Entity &other) override;

protected:

  std::shared_ptr<Item> item;
};

#endif

