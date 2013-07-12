#ifndef BARFOOS_ITEM_H
#define BARFOOS_ITEM_H

#include "common.h"
#include "icolor.h"

#include "properties.h"

class World;
class Mob;
class Entity;
class Cell;
class Gfx;
class Game;

enum class InventorySlot : size_t;

struct ItemProperties : public Properties {
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
  
  float cooldown = 1.0;
  
  float durability = 10;
  float useDurability = 1.0;
  float equipDurability = 0.0;
  
  bool canUseCell = false;
  bool canUseEntity = false;
  bool canUseNothing = false;
  
  float breakBlockStrength = 0.0;
  
  std::string replacement;
  
  // std::string placeEntity = "";
  // bool destroyBlock = false;
  std::string spawnProjectile = "";
 
  virtual void ParseProperty(const std::string &name) override;
};

void LoadItems();
const ItemProperties *getItem(const std::string &name);

class Item final {
public:

  Item(const std::string &type);
  virtual ~Item();

  void Update(Game &game);
  
  void Draw(Gfx &gfx, bool left);
  void DrawIcon(Gfx &gfx, const Point &pos) const;
  void DrawSprite(Gfx &gfx, const Vector3 &pos) const;
  
  bool CanUse(Game &game) const;
  void StartCooldown(Game &game);
  
  void UseOnEntity(Game &game, Mob &user, size_t ent);
  void UseOnCell(Game &game, Mob &user, Cell *cell, Side side);
  void UseOnNothing(Game &game, Mob &user);

  float GetRange()                      const { return this->properties->range; }  
  bool IsTwoHanded()                    const { return this->properties->twoHanded; }
  
  bool IsEquippable(InventorySlot slot) const { return this->properties->equippable & 1<<(size_t)slot; }
  uint32_t GetEquippableSlots()         const { return this->properties->equippable; }
  bool IsEquipped()                     const { return isEquipped; }
  
  void SetEquipped(bool equipped)             { this->isEquipped = equipped; }
  
  bool IsRemovable()                    const { return isRemovable; }
  
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

