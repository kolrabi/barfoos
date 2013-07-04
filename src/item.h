#ifndef BARFOOS_ITEM_H
#define BARFOOS_ITEM_H

#include "common.h"
#include "icolor.h"

class World;
class Mob;
class Entity;
class Cell;
class Gfx;
class Game;

struct ItemProperties {
  // rendering
  Sprite sprite;
  size_t equipAnim;
  IColor light;
  bool flicker = false;

  // gameplay
  float range = 5.0;
  float damage = 1.0;
  float armor = 0.0;
  
  uint32_t equippable = 0;
  bool twoHanded = false;
  
  float cooldown = 1.;
  
  float durability = 10;
  float useDurability = 1.0;
  float equipDurability = 0.0;
  
  std::string replacement;
  
  // std::string placeEntity = "";
  // bool destroyBlock = false;
  // std::string spawnProjectile = "";
  
  ItemProperties();
  ItemProperties(FILE *f);
};

void LoadItems();
const ItemProperties *getItem(const std::string &name);

class Item final {
public:

  Item(const std::string &type);
  virtual ~Item();

  bool CanUse(Game &game) const;
  void StartCooldown(Game &game);
  
  void UseOnEntity(Game &game, Mob &user, size_t ent);
  void UseOnCell(Game &game, Mob &user, Cell *cell, Side side);
  void UseOnNothing(Game &game, Mob &user);
  void Draw(Gfx &gfx, bool left);
  void DrawIcon(Gfx &gfx, const Point &pos) const;
  void DrawSprite(Gfx &gfx, const Vector3 &pos) const;

  void Update(Game &game);

  float GetRange() const { return this->properties->range; }  
  uint32_t GetEquippableSlots() { return this->properties->equippable; }
  bool IsTwoHanded() { return this->properties->twoHanded; }
  
  bool IsEquipped() const { return isEquipped; }
  void SetEquipped(bool equipped) { this->isEquipped = equipped; }
  
  bool IsRemovable() const { return isRemovable; }
  
  const ItemProperties *GetProperties() const { return this->properties; }
  virtual std::shared_ptr<Item> Combine(const std::shared_ptr<Item> &other) {
    (void)other;
    return nullptr;
  }
  
protected:

  const ItemProperties *properties;
  
  // lifecycle management
  bool isRemovable;

  // rendering
  Sprite sprite;
  float cooldownFrac;

  // gameplay
  float durability;
  bool isEquipped;
  float nextUseT;
 
  
  // bool isBlessed;
  // bool isCursed;
  // int modifier;
};

#endif

