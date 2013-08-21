#ifndef BARFOOS_ITEM_H
#define BARFOOS_ITEM_H

#include "common.h"
#include "weighted_map.h"

#include "itemproperties.h"

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

  bool UseOnEntity(RunningState &state, Mob &user, uint32_t ent);
  bool UseOnCell(RunningState &state, Mob &user, Cell *cell, Side side);
  bool UseOnNothing(RunningState &state, Mob &user);

  uint32_t GetDurability()              const { return this->durability; }
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
  bool IsInited()                       const { return this->initDone; }

  std::string GetDisplayName(bool capitalize = false)          const;
  Stats GetDisplayStats()               const;
  uint32_t GetAmount()                  const { return this->amount; }
  void DecAmount()                            { if (this->amount > 1) this->amount --; }
  void IncAmount()                            { this->amount ++; }
  void AddAmount(int amt);
  void SetAmount(int amt)                     { this->amount = amt; }
  void SetUnlockID(uint32_t id)               { this->unlockID = id; }

  bool IsConsumable()                   const { return this->properties->onConsumeEffect != "" || this->properties->onConsumeResult != "" || this->properties->onConsumeTeleport; }

  bool IsRemovable()                    const { return isRemovable || this->amount == 0; }

  const ItemProperties &GetProperties() const { return *this->properties; }
  const EffectProperties &GetEffect()   const { return *this->effect; }

  std::shared_ptr<Item> Combine(const std::shared_ptr<Item> &other);
  std::shared_ptr<Item> Consume(RunningState &state, Entity &user);

  void ModifyStats(Stats &stats, bool forceEquipped = false) const;

  void ReplaceWith(const std::string &type);

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

  bool typeIdentified, lastTypeIdentified;
  bool itemIdentified;

  uint32_t amount;
  uint32_t unlockID;

  friend Serializer   &operator << (Serializer &ser, const Item &item);
  friend Deserializer &operator >> (Deserializer &ser, Item *&item);
  friend Deserializer &operator >> (Deserializer &ser, Inventory &inventory);
};

#endif

