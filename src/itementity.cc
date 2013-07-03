#include "itementity.h"
#include "item.h"

#include "game.h"
#include "gfx.h"
#include "world.h"

ItemEntity::ItemEntity(const std::shared_ptr<Item> &item) : 
  Mob("item"),
  item(item) {
}

ItemEntity::~ItemEntity() {
}

void ItemEntity::Draw(Gfx &gfx) const {
  // Entity::Draw();
  
  IColor light = Game::Instance->GetWorld()->GetLight(this->cellPos);
  gfx.SetColor(light);
  
  this->item->DrawSprite(gfx, this->aabb.center);
}

void ItemEntity::OnUse(Entity &other) {
  if (!this->removable && other.AddToInventory(this->item)) {
    this->removable = true;
  }
}
