#ifndef BARFOOS_ITEM_H
#define BARFOOS_ITEM_H

#include "weighted_map.h"
#include "itemproperties.h"

#include "entity.pb.h"

class Inventory;

enum class Beatitude : int8_t {
  Normal = 0,
  Cursed = -1,
  Blessed = 1
};

class Item final {
public:

  Item(const std::string &type);
  Item(const Item_Proto &proto);
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

  float GetDurability()                 const { return this->proto.durability(); }
  float GetRange()                      const { return this->properties->range * this->effect->range; }
  float GetDamage()                     const { return this->properties->damage * this->effect->damage; }
  Element GetElement()                  const { return this->effect->element; }
  float GetCooldown()                   const { return this->properties->cooldown * this->effect->cooldown; }
  float GetBreakBlockStrength()         const { return this->properties->breakBlockStrength * this->effect->breakBlockStrength; }
  float GetKnockback()                  const { return this->properties->knockback * this->effect->knockback; }

  bool IsTwoHanded()                    const { return this->properties->twoHanded; }

  bool IsEquippable(InventorySlot slot) const { return this->properties->equippable & 1<<(size_t)slot; }
  uint32_t GetEquippableSlots()         const { return this->properties->equippable; }
  bool IsEquipped()                     const { return this->proto.is_equipped(); }

  void SetEquipped(bool equipped);
  bool IsCursed()                       const { return this->GetBeatitude() == Beatitude::Cursed; }
  bool CanStack(const Item &other)      const;
  bool IsInited()                       const { return this->proto.is_init_done(); }

  std::string GetDisplayName(bool capitalize = false)          const;
  const std::string &GetType()          const { return this->properties->name; }
  Stats GetDisplayStats()               const;
  void SetAmount(int amt)                     { this->proto.set_amount(amt); }
  uint32_t GetAmount()                  const { return this->proto.amount(); }
  void DecAmount()                            { if (this->GetAmount() >= 1) this->SetAmount(this->GetAmount()-1); }
  void IncAmount()                            { this->SetAmount(this->GetAmount()+1); }
  void AddAmount(int amt);
  void SetUnlockID(uint32_t id)               { this->proto.set_unlock_id(id); }
  void SetCharging(bool charging)             { this->proto.set_is_charging(charging); if (!charging) this->proto.set_charge_time(0.0); }
  bool IsCharging()                     const { return this->proto.is_charging(); }
  float GetCharge()                     const { return this->IsCharging() ? this->proto.charge_time() : 0.0; }

  bool IsConsumable()                   const { return this->properties->onConsumeEffect != "" || this->properties->onConsumeResult != "" || this->properties->onConsumeTeleport; }
  bool IsRemovable()                    const { return isRemovable || this->GetAmount() == 0; }
  bool NeedsChargeUp()                  const { return this->properties->chargeTime > 0.0; }
  float GetCooldownFrac()               const { return this->proto.cooldown_frac(); }

  const ItemProperties &GetProperties() const { return *this->properties; }
  const EffectProperties &GetEffect()   const { return *this->effect; }
  int32_t GetModifier()                 const { return this->proto.modifier(); }
  Beatitude GetBeatitude()              const { return (Beatitude)this->proto.beatitude(); }

  std::shared_ptr<Item> Combine(const std::shared_ptr<Item> &other);
  std::shared_ptr<Item> Consume(RunningState &state, Entity &user);

  void ModifyStats(Stats &stats, bool forceEquipped = false) const;

  void ReplaceWith(const std::string &type);

  const Item_Proto &GetProto() const { return this->proto; }

protected:

  Item_Proto proto;

  const ItemProperties *properties;
  const EffectProperties *effect;
  const Texture *durabilityTex;

  // lifecycle management
  bool isRemovable;

  // rendering
  Sprite sprite;

  // gameplay
  bool typeIdentified;

  friend Serializer   &operator << (Serializer &ser, const Item &item);
  friend Deserializer &operator >> (Deserializer &ser, Item *&item);
  friend Deserializer &operator >> (Deserializer &ser, Inventory &inventory);
};

#endif

