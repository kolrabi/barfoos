#include "itementity.h"
#include "item.h"

#include "game.h"
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

void ItemEntity::Start(Game &game, size_t id) {
  Mob::Start(game, id);
  this->startT += game.GetRandom().Float() * Const::pi * 2;
}

void ItemEntity::Update(Game &game) {
  Mob::Update(game);
  
  this->yoffset = std::cos(game.GetTime() - this->startT) * 0.125 + 0.125;

  this->item->Update(game);
  if (this->item->IsRemovable()) {
    this->removable = true;
  }
}
void ItemEntity::Draw(Gfx &gfx) const {
  Entity::Draw(gfx);
  
  gfx.SetColor(this->cellLight);
  
  this->item->DrawSprite(gfx, this->aabb.center + Vector3(0,yoffset,0));
}

void ItemEntity::OnUse(Game &game, Entity &other) {
  (void)game;
  if (!this->removable && other.GetInventory().AddToBackpack(this->item)) {
    this->removable = true;
  }
}
