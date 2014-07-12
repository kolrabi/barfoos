#ifndef BARFOOS_INVENTORY_H
#define BARFOOS_INVENTORY_H

#include "util/icolor.h"

#include <unordered_map>
#include <vector>
#include <utility>

//                 x  x  x  x
//   8  0          x  x  x  x
//   4  1  5       x  x  x  x
//   6  2  7       x  x  x  x
//      3          x  x  x  x
//                 x  x  x  x

/** Enum to identify inventory slots. */
enum class InventorySlot : uint8_t {
  Helmet     =  0,
  Armor      =  1,
  Greaves    =  2,
  Boots      =  3,
  LeftHand   =  4,
  RightHand  =  5,
  LeftRing   =  6,
  RightRing  =  7,
  Amulet     =  8,

  GemFire    =  9,
  GemWater   = 10,
  GemEarth   = 11,
  GemWind    = 12,
  GemLife    = 13,
  Quiver     = 14,
  Purse      = 15,

  Backpack0  = 16,
  Backpack1,
  Backpack2,
  Backpack3,
  Backpack4,
  Backpack5,
  Backpack6,
  Backpack7,
  Backpack8,
  Backpack9,
  Backpack10,
  Backpack11,
  Backpack12,
  Backpack13,
  Backpack14,
  Backpack15,

  End
};

namespace std { template<> struct hash<InventorySlot> {
  size_t operator()(const InventorySlot &slot) const { return (size_t)slot; }
}; }

class Inventory final {
public:

  Inventory();

  void                    Update(RunningState &state, Entity &owner);

  std::shared_ptr<Item> & operator[](InventorySlot slot);
  std::shared_ptr<Item>   operator[](InventorySlot slot) const;

  bool                    AddToBackpack(const std::shared_ptr<Item> &item);
  void                    AddToInventory(const std::shared_ptr<Item> &item, InventorySlot slot);
  void                    Equip(const std::shared_ptr<Item> &item, InventorySlot slot);

  void                    Drop(RunningState &state, Entity &owner);
  void DropItem(const std::shared_ptr<Item> &item);
  void ConsumeItem(InventorySlot slot, Entity &user);

  IColor GetLight() const;
  uint32_t                  GetGold()                         const;
  uint32_t                  GetGems(Element e)                const;
  void                      RemoveGem(Element e);

  void ModifyStats(Stats &stats) const;

  void Clear() { this->inventory.clear(); }

private:

  void DropItem(RunningState &state, Entity &owner, const std::shared_ptr<Item> &item);
  void Stack(InventorySlot slot, const std::shared_ptr<Item> &item);

  std::unordered_map<InventorySlot, std::shared_ptr<Item>> inventory;
  std::vector<std::shared_ptr<Item>> overflow;
  std::vector<std::pair<InventorySlot, std::shared_ptr<Item>>> equipped;
  std::vector<std::pair<InventorySlot, std::shared_ptr<Item>>> unequipped;
  std::vector<std::pair<InventorySlot, size_t>> consumed;

  float lastT;
};

#endif

