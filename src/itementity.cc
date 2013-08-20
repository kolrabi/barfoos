#include "itementity.h"
#include "item.h"
#include "player.h"

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
  item(item),
  yoffset(0.5) {
}

ItemEntity::ItemEntity(Deserializer &deser) : 
  Mob("item", deser),
  yoffset(0.5) {
  Item *item;
  deser >> item;
  this->item = std::shared_ptr<Item>(item);
}

ItemEntity::~ItemEntity() {
}

void ItemEntity::Start(RunningState &state, uint32_t id) {
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
  
  gfx.SetLight(this->cellLight);
  gfx.SetColor(IColor(255,255,255));
  
  this->item->DrawSprite(gfx, this->aabb.center + Vector3(0,yoffset,0));
}

void ItemEntity::OnUse(RunningState &state, Entity &other) {
  if (!this->removable && other.GetInventory().AddToBackpack(this->item)) {
    if (other.GetId() == state.GetPlayer().GetId()) {
      state.GetPlayer().AddMessage("You pick up the "+this->item->GetDisplayName()+".");
    }
    this->removable = true;
  }
}

void ItemEntity::Serialize(Serializer &ser) const {
  Mob::Serialize(ser);
  ser << *this->item;
}
