#ifndef BARFOOS_ITEM_H
#define BARFOOS_ITEM_H

#include "common.h"

class World;
class Mob;
class Entity;
class Cell;

struct ItemProperties {
  // rendering
  unsigned int texture = 0;
  size_t frames = 1;
  std::vector<Animation> anims;
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
  
  // std::string placeEntity = "";
  // bool destroyBlock = false;
  // std::string spawnProjectile = "";
  
  ItemProperties();
  ItemProperties(FILE *f);
};

void LoadItems();
const ItemProperties *getItem(const std::string &name);

class Item {
public:

  Item(const std::string &type);
  virtual ~Item();

  virtual bool CanUse() const;
  virtual void StartCooldown();
  
  virtual void UseOnEntity(const std::shared_ptr<Entity> &ent);
  virtual void UseOnCell(Cell *cell, Side side);
  virtual void Draw(bool left);
  
  virtual void Update(float t);

  float GetRange() const { return this->properties->range; }  
  uint32_t GetEquippableSlots() { return this->properties->equippable; }
  bool IsTwoHanded() { return this->properties->twoHanded; }
  
  bool IsEquipped() const { return isEquipped; }
  void SetEquipped(bool equipped) { this->isEquipped = equipped; }
  
  bool IsRemovable() const { return isRemovable; }
  
  virtual unsigned int GetIconTexture() const { return this->properties->texture; }
  
  const ItemProperties *GetProperties() const { return this->properties; }
  virtual std::shared_ptr<Item> Combine(const std::shared_ptr<Item> &other) {
    (void)other;
    return nullptr;
  }
  
  void DrawIcon(const Point &p) const;

protected:

  const ItemProperties *properties;

  size_t animation;
  float frame;  

  float lastT;
  float nextUseT;
  float durability;
  bool isEquipped;
  
  bool isRemovable;
  
  // bool isBlessed;
  // bool isCursed;
  // int modifier;
};

#endif

