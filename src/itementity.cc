#include "itementity.h"
#include "item.h"

#include "runningstate.h"
#include "gfx.h"
#include "world.h"

ItemEntity::ItemEntity(const std::string &itemName) : 
  Mob("item"),
  item(new Item(itemName)),
  yoffset(0.5)  {
}

ItemEntity::ItemEntity(const std::shared_ptr<Item> &item) : 
  Mob("item"),
  item(item) {
}

ItemEntity::~ItemEntity() {
}

void ItemEntity::Start(RunningState &state, size_t id) {
  Mob::Start(state, id);
  this->startT += state.GetRandom().Float() * Const::pi * 2;
}

void ItemEntity::Update(RunningState &state) {
  Mob::Update(state);
  
  this->yoffset = std::cos(state.GetGame().GetTime() - this->startT) * 0.125 + 0.125;

  this->item->Update(state);
  if (this->item->IsRemovable()) {
    this->removable = true;
  }
}
void ItemEntity::Draw(Gfx &gfx) const {
  Entity::Draw(gfx);
  
  gfx.SetColor(this->cellLight);
  
  this->item->DrawSprite(gfx, this->aabb.center + Vector3(0,yoffset,0));
}

void ItemEntity::OnUse(RunningState &, Entity &other) {
  if (!this->removable && other.GetInventory().AddToBackpack(this->item)) {
    this->removable = true;
  }
}
