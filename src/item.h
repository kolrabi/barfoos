#ifndef BARFOOS_ITEM_H
#define BARFOOS_ITEM_H

#include "common.h"
#include "icolor.h"
#include "sprite.h"

#include "weighted_map.h"

#include "effect.h"
#include "properties.h"

struct ItemProperties : public Properties {
  std::string name = "<item>";
  std::string identifiedName = "<item>";
  std::string unidentifiedName = "<item>";
  
  bool isPotion = false;
  
  // rendering
  Sprite sprite = Sprite();
  size_t equipAnim = 0;
  IColor light {0,0,0};
  bool flicker = false;

  // gameplay
  float range = 5.0;
  float damage = 1.0;
  float knockback = 0.0;
  
  int eqAddStr = 0;
  int eqAddDex = 0;
  int eqAddAgi = 0;
  int eqAddDef = 0;
  int eqAddHP  = 0;

  int uneqAddStr = 0;
  int uneqAddDex = 0;
  int uneqAddAgi = 0;
  int uneqAddDef = 0;
  int uneqAddHP  = 0;
  
  std::string weaponClass = "";
  
  uint32_t equippable = 0;
  bool twoHanded = false;
  
  float cooldown = 1.0;
  
  float durability = 10;
  float useDurability = 0.0;
  float equipDurability = 0.0;
  
  bool canUseCell = false;
  bool canUseEntity = false;
  bool canUseNothing = false;
  bool noModifier = false;
  bool noBeatitude = false;
  
  std::string onCombineEffect = "";
  std::string onConsumeEffect = "";
  std::string onConsumeResult = "";
  std::string onConsumeAddBuff = "";

  weighted_map<std::string> onHitAddBuff;
  
  float breakBlockStrength = 0.0;
  
  std::string replacement = "";
  
  // std::string placeEntity = "";
  std::string spawnProjectile = "";
  
  weighted_map<std::string> effects = weighted_map<std::string>();
  bool stackable = false;
 
  virtual void ParseProperty(const std::string &name) override;
};

void LoadItems(Game &game);
const ItemProperties &getItem(const std::string &name);

enum class Beatitude : int8_t {
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

  void Update(RunningState &state);
  
  void Draw(Gfx &gfx, bool left);
  void DrawIcon(Gfx &gfx, const Point &pos) const;
  void DrawSprite(Gfx &gfx, const Vector3 &pos) const;
  
  bool CanUse(RunningState &state) const;
  void StartCooldown(RunningState &state, Entity &user, bool damage = true);
  
  void UseOnEntity(RunningState &state, Mob &user, size_t ent);
  void UseOnCell(RunningState &state, Mob &user, Cell *cell, Side side);
  void UseOnNothing(RunningState &state, Mob &user);

  float GetRange()                      const { return this->properties->range * this->effect->range; }  
  float GetDamage()                     const { return this->properties->damage * this->effect->damage; }  
  Element GetElement()                  const { return this->effect->element; }  
  float GetCooldown()                   const { return this->properties->cooldown * this->effect->cooldown; }
  float GetBreakBlockStrength()         const { return this->properties->breakBlockStrength * this->effect->breakBlockStrength; }
  float GetKnockback()                  const { return this->properties->knockback * this->effect->knockback; }
  
  bool IsTwoHanded()                    const { return this->properties->twoHanded; }
  
  bool IsEquippable(InventorySlot slot) const { return this->properties->equippable & 1<<(size_t)slot; }
  uint32_t GetEquippableSlots()         const { return this->properties->equippable; }
  bool IsEquipped()                     const { return isEquipped; }
  
  void SetEquipped(bool equipped);
  bool IsCursed()                       const { return this->beatitude == Beatitude::Cursed; }
  bool CanStack(const Item &other)      const;
  
  std::string GetDisplayName(bool capitalize = false)          const;
  Stats GetDisplayStats()               const;
  size_t GetAmount()                    const { return this->amount; }
  void DecAmount()                            { if (this->amount > 1) this->amount --; }
  void IncAmount()                            { this->amount ++; }
  void AddAmount(int amt);
  void SetAmount(int amt)                     { this->amount = amt; }
  
  bool IsConsumable()                   const { return this->properties->onConsumeEffect != "" || this->properties->onConsumeResult != ""; }
  
  bool IsRemovable()                    const { return isRemovable || this->amount == 0; }
  
  const ItemProperties &GetProperties() const { return *this->properties; }
  const EffectProperties &GetEffect()   const { return *this->effect; }
  
  virtual std::shared_ptr<Item> Combine(const std::shared_ptr<Item> &other);
  virtual std::shared_ptr<Item> Consume(RunningState &state, Entity &user);
  
  void ModifyStats(Stats &stats, bool forceEquipped = false) const;
  
protected:

  bool initDone;

  const ItemProperties *properties;
  const EffectProperties *effect;
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

  bool identified;
  
  size_t amount;
  
  friend Serializer &operator << (Serializer &ser, const Item &item);
  friend Deserializer &operator >> (Deserializer &ser, Item *&item);
  friend Deserializer &operator >> (Deserializer &ser, Inventory &inventory);
};

#endif

