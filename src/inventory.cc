#include "inventory.h"

#include "runningstate.h"
#include "world.h"

#include "entity.h"
#include "item.h"
#include "itementity.h"

#include "simplex.h"

#include "serializer.h"

Inventory::Inventory() :
  inventory(),
  overflow(0),
  lastT(0.0)
{}

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
  if (item->GetProperties().stackable) {
    while(i < InventorySlot::End) {
      if (self[i] && item->GetProperties() == self[i]->GetProperties()) {
        self[i]->AddAmount(item->GetAmount());
        return true;
      }
      i = InventorySlot((size_t)i + 1);
    }
    i = InventorySlot::Backpack0;
  }
  
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
  if (item->GetAmount() > 1) {

    std::shared_ptr<Item> rest(new Item(item->GetProperties().name));
    rest->AddAmount(item->GetAmount() - 2);
  
    item->AddAmount(-item->GetAmount() + 1);
    
    if (!this->AddToBackpack(rest)) {
      this->DropItem(rest);
    }
  }
  
  if (!this->inventory[slot]) {
    // target slot is free
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
  std::shared_ptr<Item> combo(item->Combine(this->inventory[slot]));
  
  if (combo) {
    DropItem(item);
  }
  
  if (combo) {
    // replace existing item with combination
    this->inventory[slot] = nullptr;
    this->Equip(combo, slot);
    return true;
  }
  
  if (self[slot]->IsCursed() && slot < InventorySlot::Backpack0) {
    if (!this->AddToBackpack(item)) {
      this->DropItem(item);
    }
    return false;
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
    if (equip) this->equipped.push_back({slot, item});
    else this->unequipped.push_back({slot, item});
  }
}

void
Inventory::Update(RunningState &state, Entity &owner) {
  this->lastT = state.GetGame().GetTime();
  
  for (auto &i:overflow) {
    i->Update(state);
    DropItem(state, owner, i);
  }
  overflow.clear();

  for (auto i:consumed) {
    if (!self[i]) continue;
    self[i] = self[i]->Consume(state, owner);
  }
  consumed.clear();
  
  for (auto &i:equipped) {
    owner.OnEquip(state, *i.second, i.first, true);
  }
  equipped.clear();

  for (auto &i:unequipped) {
    owner.OnEquip(state, *i.second, i.first, false);
  }
  unequipped.clear();
  
  for (auto &i:inventory) {
    if (i.second) {
      i.second->Update(state);
      if (i.second->IsRemovable()) {
        self[i.first] = nullptr;
      }
    }
  }
}

/** Drop all items on the floor. 
*/
void
Inventory::Drop(RunningState &state, Entity &owner) {
  for (auto &item : this->inventory) {
    if (item.second) {
      DropItem(state, owner, item.second);
    }
    item.second = nullptr;
  }
}

void 
Inventory::DropItem(const std::shared_ptr<Item> &item) {
  overflow.push_back(item);
}

void 
Inventory::ConsumeItem(InventorySlot slot) {
  consumed.push_back(slot);
}

void
Inventory::DropItem(RunningState &state, Entity &owner, const std::shared_ptr<Item> &item) {
  if (!item || item->IsRemovable()) return;

  while(item->GetAmount() > 1) {
    DropItem(state, owner, std::shared_ptr<Item>(new Item(item->GetProperties().name)));
    item->DecAmount();
  }  
  
  ItemEntity *entity = new ItemEntity(item);
  entity->SetPosition(owner.GetPosition());
  
  Vector3 offset(owner.GetForward() + state.GetRandom().Vector() * owner.GetAABB().extents);
  
  entity->SetPosition(state.GetWorld().MoveAABB(entity->GetAABB(), offset + entity->GetPosition()));
  entity->AddVelocity(state.GetRandom().Vector()*1);
  entity->AddVelocity(owner.GetForward() + Vector3(0,1,0)*10);
  
  state.AddEntity(entity);
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

void 
Inventory::ModifyStats(Stats &stats) const {
  for (auto item : this->inventory) {
    if (!item.second) continue;
    item.second->ModifyStats(stats);
  }
}

Serializer &operator << (Serializer &ser, const Inventory &inventory) {
  size_t count = 0;
  for (auto item : inventory.inventory) {
    if (item.second) count ++;
  }
  
  ser << count;
  for (auto item : inventory.inventory) {
    if (item.second) ser << *item.second;
  }
  return ser;
}

