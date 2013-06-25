#include "itementity.h"
#include "item.h"

#include <GL/glfw.h>

ItemEntity::ItemEntity(const std::shared_ptr<Item> &item) : 
  Mob("item"),
  item(item) {
}

ItemEntity::~ItemEntity() {
}

void ItemEntity::Draw() const {
  Entity::Draw();
  
  glColor3ub(light.r, light.g, light.b);
  this->item->DrawSprite(this->aabb.center, this->properties->w/2, this->properties->h/2);
}

void ItemEntity::OnUse(Entity &other) {
  if (!this->removable && other.AddToInventory(this->item)) {
    this->removable = true;
  }
}
