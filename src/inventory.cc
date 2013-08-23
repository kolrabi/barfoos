#include "inventory.h"

#include "runningstate.h"
#include "world.h"

#include "entity.h"
#include "item.h"
#include "itementity.h"

#include "simplex.h"

#include "serializer.h"
#include "deserializer.h"

Inventory::Inventory() :
  inventory(),
  overflow(0),
  lastT(0.0)
{}

std::shared_ptr<Item> &
Inventory::operator[](InventorySlot slot) {
  return inventory[slot];
}

std::shared_ptr<Item>
Inventory::operator[](InventorySlot slot) const {
  if (inventory.find(slot) == inventory.end()) return std::shared_ptr<Item>(nullptr);
  return inventory.at(slot);
}

/** Add item to first free slot in backpack.
  */
bool
Inventory::AddToBackpack(const std::shared_ptr<Item> &item) {
  if (!item) return true;

  //Log("AddToBackpack %s %u %s\n", item->GetDisplayName().c_str(), item->GetAmount(), item->GetType().c_str());

  if (item->GetType()=="gold") {
    // gold goes to the purse
    if (self[InventorySlot::Purse]) {
      self[InventorySlot::Purse]->AddAmount(item->GetAmount());
      return true;
    } else {
      self[InventorySlot::Purse] = item;
    }

    return true;
  }

  InventorySlot i = InventorySlot::Backpack0;
  while(i < InventorySlot::End) {
    if (self[i] && self[i]->CanStack(*item)) {
      self[i]->AddAmount(item->GetAmount());
      return true;
    }
    i = InventorySlot((size_t)i + 1);
  }

  i = InventorySlot::Backpack0;
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
void
Inventory::AddToInventory(const std::shared_ptr<Item> &item, InventorySlot slot) {
  //Log("AddToInventory %s %u\n", item->GetDisplayName().c_str(), item->GetAmount());

  if (!self[slot]) {
    // target slot is free
    if (slot >= InventorySlot::Backpack0 || item->IsEquippable(slot)) {
      // place item
      this->Equip(item, slot);
      return;
    }

    // not allowed in that slot, put somewhere
    this->AddToBackpack(item);
    return;
  }

  // slot is occupied, try stacking things together
  if (self[slot]->CanStack(*item)) {
    //Log("Can Stack!\n");
    self[slot]->AddAmount(item->GetAmount());
    item->SetAmount(0);
    return;
  } else if (item->GetAmount() > 1) {
    //Log("Cannot Stack!\n");
    // can't be stacked, put all but one from stack into backpack
    std::shared_ptr<Item> rest(new Item(item->GetProperties().name));
    rest->SetAmount(item->GetAmount()-1);
    item->SetAmount(1);

    //Log("%u %u\n", item->GetAmount(), rest->GetAmount());

    //Log("Putting %u back into backpack\n", rest->GetAmount());
    if (!this->AddToBackpack(rest)) {
      this->DropItem(rest);
    }
  }

  // combine
  std::shared_ptr<Item> combo(item->Combine(this->inventory[slot]));

  if (combo) {
    // replace existing item with combination
    Log("Combined to %s\n", combo->GetDisplayName().c_str());
    this->inventory[slot] = nullptr;
    this->Equip(combo, slot);
    if (!item->IsRemovable()) this->AddToBackpack(item);
    return;
  }

  if (self[slot]->IsCursed() && slot < InventorySlot::Backpack0) {
    Log("Target is cursed, putting item in backpack\n");
    if (!this->AddToBackpack(item)) {
      this->DropItem(item);
    }
    return;
  }

  // try to put existing item in backpack if enough room
  Log("Putting existing item in the backpack\n");
  std::shared_ptr<Item> oldItem = self[slot];
  self[slot] = nullptr;
  this->Equip(item, slot);
  this->AddToBackpack(oldItem);
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

  if (item && item->IsEquipped() != equip) {
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

  for (auto &i:consumed) {
    if (!self[i.first]) continue;
    Entity *user = state.GetEntity(i.second);
    if (!user) continue;

    self[i.first] = self[i.first]->Consume(state, *user);
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
Inventory::ConsumeItem(InventorySlot slot, Entity &user) {
  consumed.push_back(std::pair<InventorySlot, size_t>(slot, user.GetId()));
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
    light = light + item.second->GetEffect().light;
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

uint32_t
Inventory::GetGold() const {
  if (!self[InventorySlot::Purse]) return 0;
  return self[InventorySlot::Purse]->GetAmount();
}

Serializer &operator << (Serializer &ser, const Inventory &inventory) {
  uint32_t count = 0;
  for (auto item : inventory.inventory) {
    if (item.second) count ++;
  }

  ser << count;
  for (auto item : inventory.inventory) {
    if (item.second) ser << (uint32_t)item.first << *item.second;
  }
  return ser;
}

Deserializer &operator >> (Deserializer &deser, Inventory &inventory) {
  inventory.inventory.clear();

  uint32_t count;
  deser >> count;

  for (size_t i = 0; i<count; i++) {
    uint32_t slot;
    deser >> slot;

    Item *item;
    deser >> item;

    inventory.inventory[(InventorySlot)slot] = std::shared_ptr<Item>(item);
    item->isEquipped = slot < (int)InventorySlot::Backpack0;
  }
  return deser;
}
