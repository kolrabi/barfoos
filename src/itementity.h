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
  virtual void Start(RunningState &state, size_t id) override;
  virtual void Update(RunningState &state) override;
  
  virtual void OnUse(RunningState &state, Entity &other) override;

protected:

  std::shared_ptr<Item> item;
  float yoffset;
};

#endif

