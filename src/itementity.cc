#include "itementity.h"
#include "item.h"

ItemEntity::ItemEntity(const std::shared_ptr<Item> &item) : 
  Mob("item"),
  item(item) {
}

ItemEntity::~ItemEntity() {
}

void ItemEntity::Draw() {
  Entity::Draw();
  this->item->DrawSprite(this->aabb.center, this->aabb.extents.x, this->aabb.extents.y);
}

void ItemEntity::OnUse(Entity *other) {
  if (!this->removable && other->AddToInventory(this->item)) {
    this->removable = true;
  }
}
