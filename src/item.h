#ifndef BARFOOS_ITEM_H
#define BARFOOS_ITEM_H

#include "common.h"
#include "icolor.h"
#include "sprite.h"

#include "properties.h"

struct ItemProperties : public Properties {
  std::string name = "<item>";
  
  // rendering
  Sprite sprite = Sprite();
  size_t equipAnim = 0;
  IColor light {0,0,0};
  bool flicker = false;

  // gameplay
  float range = 5.0;
  float damage = 1.0;
  
  int eqAddStr = 0;
  int eqAddDex = 0;
  int eqAddAgi = 0;
  int eqAddDef = 0;

  int uneqAddStr = 0;
  int uneqAddDex = 0;
  int uneqAddAgi = 0;
  int uneqAddDef = 0;
  
  std::string weaponClass = "";
  
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
  
  std::string replacement = "";
  
  // std::string placeEntity = "";
  std::string spawnProjectile = "";
 
  virtual void ParseProperty(const std::string &name) override;
};

void LoadItems();
const ItemProperties &getItem(const std::string &name);

enum class Beatitude : int {
  Normal = 0,
  Cursed = -1,
  Blessed = 1
};

class Item final {
public:

  Item(const std::string &type);
  Item(const Item &) = default;
  virtual ~Item();
  
  Item &operator=(const Item &) = default;

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
  
  const ItemProperties &GetProperties() const { return *this->properties; }
  
  virtual std::shared_ptr<Item> Combine(const std::shared_ptr<Item> &other) {
    (void)other;
    return nullptr;
  }
  
  void ModifyStats(Stats &stats) const;
  
protected:

  bool initDone;

  const ItemProperties *properties;
  const Texture *durabilityTex;
  
  // lifecycle management
  bool isRemovable;

  // rendering
  Sprite sprite;
  float cooldownFrac;

  // gameplay
  float durability;
  bool isEquipped;
  float nextUseT;
 
  Beatitude beatitude;
  int modifier;
};

#endif

