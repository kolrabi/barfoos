#ifndef BARFOOS_ITEM_H
#define BARFOOS_ITEM_H

#include "common.h"

class World;
class Mob;
class Entity;
class Cell;

struct ItemClass {
  std::string name;
};

//                 x  x  x  x
//   8  0          x  x  x  x
//   4  1  5       x  x  x  x
//   6  2  7       x  x  x  x
//      3          x  x  x  x
//                 x  x  x  x

enum class InventorySlot {
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

  Backpack   = 16
  // ...
};

class Item {
public:

  Item();
  virtual ~Item();

  virtual bool CanUse() const;
  virtual void StartCooldown();
  
  virtual void UseOnEntity(const std::shared_ptr<Entity> &ent) = 0;
  virtual void UseOnCell(Cell *cell, Side side) = 0;
  virtual void Draw(bool left) { (void)left; }

  float GetRange() const { return range; }  
  uint32_t GetEquippableSlots() { return equippable; }
  bool IsTwoHanded() { return twoHanded; }
  
  virtual unsigned int GetIconTexture() const { return icon; }

protected:

  float range;  
  unsigned int icon;
  
  bool twoHanded;
  uint32_t equippable;
  
  float cooldown;
  float nextUseT;
};

#endif

