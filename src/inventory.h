#ifndef BARFOOS_INVENTORY_H
#define BARFOOS_INVENTORY_H

#include "common.h"
#include "icolor.h"

#include <unordered_map>
#include <utility>

//                 x  x  x  x
//   8  0          x  x  x  x
//   4  1  5       x  x  x  x
//   6  2  7       x  x  x  x
//      3          x  x  x  x
//                 x  x  x  x

/** Enum to identify inventory slots. */
enum class InventorySlot : size_t {
  Helmet     =  0, 
  Armor      =  1, 
  Greaves    =  2, 
  Boots      =  3,
  LeftHand   =  4, 
  RightHand  =  5,
  LeftRing   =  6, 
  RightRing  =  7,
  Amulet     =  8, 
  
  Reserved9  =  9,
  Reserved10 = 10,
  Reserved11 = 11,
  Reserved12 = 12,
  Reserved13 = 13,
  Reserved14 = 14,
  Reserved15 = 15,

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
  
  void Update(RunningState &state, Entity &owner);

  std::shared_ptr<Item> &operator[](InventorySlot slot);
  const std::shared_ptr<Item> &operator[](InventorySlot slot) const;

  bool AddToBackpack(const std::shared_ptr<Item> &item);
  bool AddToInventory(const std::shared_ptr<Item> &item, InventorySlot slot);
  void Equip(const std::shared_ptr<Item> &item, InventorySlot slot);
  
  void Drop(RunningState &state, Entity &owner);
  void DropItem(const std::shared_ptr<Item> &item);
  void ConsumeItem(InventorySlot slot);
  
  IColor GetLight() const;
  
  void ModifyStats(Stats &stats) const;

private:

  void DropItem(RunningState &state, Entity &owner, const std::shared_ptr<Item> &item);

  std::unordered_map<InventorySlot, std::shared_ptr<Item>> inventory;
  std::vector<std::shared_ptr<Item>> overflow;
  std::vector<std::pair<InventorySlot, std::shared_ptr<Item>>> equipped;
  std::vector<std::pair<InventorySlot, std::shared_ptr<Item>>> unequipped;
  std::vector<InventorySlot> consumed;

  float lastT;
  
  friend Serializer &operator << (Serializer &ser, const Inventory &inventory);
  friend Deserializer &operator >> (Deserializer &ser, Inventory &inventory);
};

#endif

