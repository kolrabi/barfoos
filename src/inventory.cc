#include "inventory.h"

#include "game.h"
#include "world.h"

#include "entity.h"
#include "item.h"
#include "itementity.h"

#include "simplex.h"

std::shared_ptr<Item> &
Inventory::operator[](InventorySlot slot) { 
  return inventory[slot]; 
}

const std::shared_ptr<Item> &
Inventory::operator[](InventorySlot slot) const {
  return inventory.at(slot);
}

/** Add item to first free slot in backpack.
  */
bool
Inventory::AddToBackpack(const std::shared_ptr<Item> &item) {
  InventorySlot i = InventorySlot::Backpack0;
  while(i < InventorySlot::End) {
    if (!self[i]) {
      self[i] = item;
      item->SetEquipped(false);
      return true;
    }
    
    i = InventorySlot((size_t)i + 1);
  }
  return false;
}

/** Add item to given slot if possible, or first free one.
  * Equips item if slot is not a backpack slot. Combines items if possible.
  */
bool
Inventory::AddToInventory(const std::shared_ptr<Item> &item, InventorySlot slot) {
  if (!this->inventory[slot]) {
    // target slot is not free
    if (slot >= InventorySlot::Backpack0 || item->IsEquippable(slot)) {
      // replace item
      this->Equip(item, slot);
      return true;
    } else {
      // put somewhere
      return this->AddToBackpack(item);
    }
  }

  // combine
  std::shared_ptr<Item> combo(item->Combine(self[slot]));
  
  if (!combo) {
    // if that didn't work, try the reverse
    combo = self[slot]->Combine(item);
  }
  
  if (combo) {
    // replace existing item with combination
    this->Equip(combo, slot);
    return true;
  }

  // try to put existing item in backpack if enough room
  if (this->AddToBackpack(self[slot])) {
    self[slot] = nullptr;
    this->Equip(item, slot);
    return true;
  }
  return false;
}

/** Put item in the given slot, move any existing item to backpack.
  */
void 
Inventory::Equip(const std::shared_ptr<Item> &item, InventorySlot slot) {
  bool equip = slot < InventorySlot::Backpack0;
  
  // alredy an item there?
  if (self[slot] && item) {
    self[slot]->SetEquipped(false);
    if (!this->AddToBackpack(self[slot])) {
      this->overflow.push_back(self[slot]);
    }
  }
  self[slot] = item;
  
  if (item) {
    item->SetEquipped(equip);
  }
}

void
Inventory::Update(Game &game, Entity &owner) {
  this->lastT = game.GetTime();
  
  for (auto &i:overflow) {
    DropItem(game, owner, i);
  }
  overflow.clear();
  
  for (auto &i:inventory) {
    if (i.second) {
      i.second->Update(game);
      if (i.second->IsRemovable()) {
        self[i.first] = nullptr;
      }
    }
  }
}

/** Drop all items on the floor. 
*/
void
Inventory::Drop(Game &game, Entity &owner) {
  for (auto &item : this->inventory) {
    DropItem(game, owner, item.second);
  }
}

void 
Inventory::DropItem(const std::shared_ptr<Item> &item) {
  overflow.push_back(item);
}

void
Inventory::DropItem(Game &game, Entity &owner, const std::shared_ptr<Item> &item) {
  if (!item) return;
  
  ItemEntity *entity = new ItemEntity(item);
  entity->SetPosition(owner.GetPosition());
  
  Vector3 offset(owner.GetForward() + game.GetRandom().Vector() * owner.GetAABB().extents);
  
  entity->SetPosition(game.GetWorld().MoveAABB(entity->GetAABB(), offset + entity->GetPosition()));
  entity->AddVelocity(game.GetRandom().Vector()*10);
  entity->AddVelocity(owner.GetForward() + Vector3(0,1,0)*10);
  
  game.AddEntity(entity);
}

IColor 
Inventory::GetLight() const {
  float t = lastT;
  IColor light;
  
  for (auto item : this->inventory) {
    if (!item.second || !item.second->IsEquipped()) continue;
    
    float f = 1.0;
    if (item.second->GetProperties().flicker) {
      f = simplexNoise(Vector3(t*3, 0, 0)) * simplexNoise(Vector3(t*2, -t, 0));
      f = f * 0.4 + 0.5;
    }
    light = light + item.second->GetProperties().light * f;
  }  
  return light;
}