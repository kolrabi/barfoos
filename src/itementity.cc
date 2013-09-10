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
  this->proto.set_spawn_class(uint32_t(SpawnClass::ItemEntityClass));
}

ItemEntity::ItemEntity(const std::shared_ptr<Item> &item) : 
  Mob("item"),
  item(item),
  yoffset(0.5) {
  this->proto.set_spawn_class(uint32_t(SpawnClass::ItemEntityClass));
}

ItemEntity::~ItemEntity() {
}

void ItemEntity::Start(RunningState &state, uint32_t id) {
  Mob::Start(state, id);
  this->SetStartTime(this->GetStartTime() + state.GetRandom().Float() * Const::pi * 2);
}

void ItemEntity::Continue(RunningState &state, uint32_t id) {
  Mob::Continue(state, id);
  this->item = std::shared_ptr<Item>(new Item(this->proto.item()));
}

void ItemEntity::Update(RunningState &state) {
  Mob::Update(state);
  
  this->yoffset = std::cos(state.GetGame().GetTime() - this->GetStartTime()) * 0.125 + 0.125;

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

const Entity_Proto &
ItemEntity::GetProto() {
  *this->proto.mutable_item() = this->item->GetProto();
  return Mob::GetProto();
}
