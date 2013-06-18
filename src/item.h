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
//
//  16 17 18 19   20 21 22 23 

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
  
  QuickSlot1 = 16,
  QuickSlot2 = 17,
  QuickSlot3 = 18,
  QuickSlot4 = 19,
  QuickSlot5 = 20,
  QuickSlot6 = 21,
  QuickSlot7 = 22,
  QuickSlot8 = 23,
  
  Backpack   = 24
  // ...
};

class Item {
public:

  Item();
  virtual ~Item();

  virtual void Use(Entity &entity, const Vector3 &pos, const Vector3 &dir, bool left);

  virtual void UseOnEntity(const std::shared_ptr<Entity> &ent, const Vector3 &p, bool left) = 0;
  virtual void UseOnCell(Cell *cell, Side side, bool left) = 0;
  virtual void Draw(bool left) { (void)left; }
  virtual void DrawIcon(float x, float y, float w, float h);
  virtual void DrawBillboard(const Vector3 &pos);
  
  virtual uint32_t GetEquippableSlots() { return 0; }
  virtual bool IsTwoHanded() { return false; }

protected:

  float range;  
  unsigned int icon;
  
  float cooldown;
  float nextUseT;
};

#endif

