#ifndef BARFOOS_ITEMENTITY_H
#define BARFOOS_ITEMENTITY_H

#include "common.h"

#include "mob.h"

class Item;

class ItemEntity : public Mob {
public:

  ItemEntity(const std::string &itemName);
  ItemEntity(const std::shared_ptr<Item> &item);
  ItemEntity(const Entity_Proto &proto);
  virtual ~ItemEntity();

  virtual void Draw(Gfx &gfx) const override;
  virtual void Start(RunningState &state, uint32_t id) override;
  virtual void Continue(RunningState &state, uint32_t id) override;
  virtual void Update(RunningState &state) override;

  virtual void OnUse(RunningState &state, Entity &other) override;

  const std::shared_ptr<Item> &GetItem() const { return item; }

  virtual const Entity_Proto &GetProto()                        override;

protected:

  std::shared_ptr<Item> item;
  float yoffset;
};

#endif

