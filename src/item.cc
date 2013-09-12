#include "common.h"

#include "item.h"

#include "entity.h"
#include "world.h"
#include "mob.h"
#include "runningstate.h"
#include "gfx.h"
#include "gfxview.h"
#include "projectile.h"
#include "texture.h"
#include "text.h"
#include "itementity.h"
#include "player.h"
#include "inventory.h"

#include "serializer.h"
#include "deserializer.h"

Item::Item(const std::string &type) :
  properties(&getItem(type)),
  effect(nullptr),
  durabilityTex(Texture::Get("gui/durability")),
  isRemovable(false),
  sprite(this->properties->sprite),
  typeIdentified(false)
{
  if (!this->sprite.animations.empty()) this->sprite.StartAnim(0);

  this->proto.set_durability(this->properties->durability);
}

Item::Item(const Item_Proto &proto) : 
  proto(proto),
  properties(&getItem(proto.type())),
  effect(&getEffect(proto.has_effect()?proto.effect():"")),
  durabilityTex(Texture::Get("gui/durability")),
  isRemovable(false),
  sprite(this->properties->sprite),
  typeIdentified(proto.is_item_identified())
{
}

Item::~Item() {
}

void Item::ReplaceWith(const std::string &type) {
  this->properties = &getItem(type);
  this->isRemovable = false;
  this->sprite = this->properties->sprite;
  this->typeIdentified = false;

  this->proto.set_is_init_done(false);
  this->proto.set_cooldown_frac(0.0f);
  this->proto.set_durability(this->properties->durability);
  this->proto.set_next_use_time(0.0f);
  this->proto.set_is_item_identified(false);
  this->proto.set_is_charging(false);
  this->proto.set_charge_time(0.0f);
  this->proto.set_amount(1);
  this->proto.set_unlock_id(0);

  if (!this->sprite.animations.empty()) this->sprite.StartAnim(0);
}

bool Item::CanUse(RunningState &state) const {
  return (this->GetDurability() > 0 || this->properties->durability == 0.0) &&
         this->properties->cooldown >= 0.0 &&
         this->proto.next_use_time() < state.GetGame().GetTime();
}

void Item::StartCooldown(RunningState &state, Entity &user, bool damage) {
  if (damage) {
    this->proto.set_durability(this->GetDurability() - this->properties->useDurability * this->effect->useDurability);
  }

  Stats stats = user.GetEffectiveStats();
  float cooldown = this->GetCooldown() / (1.0 + Const::AttackSpeedFactorPerAGI*stats.GetAgility());
  cooldown *= stats.GetCoolDown();
  this->proto.set_next_use_time(state.GetGame().GetTime() + cooldown);
  if (this->properties->onUseIdentify) {
    this->proto.set_is_item_identified(true);
  }

  this->proto.set_is_charging(false);
  this->proto.set_charge_time(0.0f);
}

void Item::Update(RunningState &state) {
  Game &game = state.GetGame();

  if (!this->proto.is_init_done()) {
    this->proto.set_is_init_done(true);

    if (!this->properties->noModifier) {
      this->proto.set_modifier(state.GetRandom().Integer(3) + state.GetRandom().Integer(3) - 2);
    }

    if (!this->properties->noBeatitude) {
      if (state.GetRandom().Chance(0.01)) {
        this->proto.set_beatitude(int32_t(Beatitude::Cursed));
        this->proto.set_modifier(-2);
      } else if (state.GetRandom().Chance(0.1)) {
        this->proto.set_beatitude(int32_t(Beatitude::Blessed));
        this->proto.set_modifier(2);
      }
    }

    std::string effectName = this->properties->effects.size() == 0 ? "" : this->properties->effects.select(game.GetRandom().Float01());
    if (effectName[0] == '$') {
      const std::vector<std::string> &effects = getEffectsInGroup(effectName.substr(1));
      if (effects.empty()) {
        effectName = "";
      } else {
        effectName = effects[state.GetRandom().Integer(effects.size())];
      }
    }

    this->effect = &getEffect(effectName);
    this->proto.set_effect(effectName);
    this->proto.set_durability(this->proto.durability() * this->effect->durability);
  }

  if (!this->typeIdentified && this->proto.is_item_identified()) {
    this->typeIdentified = true;
    game.SetIdentified(this->properties->name);
    state.GetPlayer().AddMessage(
      "You learnt that a " + this->properties->unidentifiedName +
      " is a " + this->properties->identifiedName +
      ((this->effect && this->properties->effects.size()==1) ? this->effect->displayName : "")
    );
  } else {
    this->typeIdentified = game.IsIdentified(this->properties->name);
  }

  // reduce durability while equipped
  if (this->IsEquipped()) {
    this->sprite.currentAnimation = this->properties->equipAnim;
    this->proto.set_durability(this->GetDurability() - this->properties->equipDurability * game.GetDeltaT() * this->effect->equipDurability);
  } else {
    this->sprite.currentAnimation = 0;
  }

  this->sprite.Update(game.GetDeltaT());

  // when broken, replace or remove
  if (this->GetDurability() <= 0 && this->properties->durability != 0.0) {
    std::string replacement = this->properties->replacement;
    state.GetPlayer().AddMessage("Your "+this->GetDisplayName()+" broke...");
    if (replacement != "") {
      this->ReplaceWith(replacement);
      return;
    } else {
      this->DecAmount();
      if (this->GetAmount() > 0) {
        this->proto.set_durability(this->properties->durability);
      }
    }
  }

  // cooldown fraction: 1.0 full cooldown left, 0.0 cooldown over
  this->proto.set_cooldown_frac(std::max(0.0f, (this->proto.next_use_time() - game.GetTime())/this->properties->cooldown));

  if (this->IsCharging() && this->NeedsChargeUp()) {
    this->proto.set_charge_time(this->proto.charge_time() + game.GetDeltaT() / this->properties->chargeTime);
    if (this->proto.charge_time() > 1.0) this->proto.set_charge_time(1.0f);
  } else {
    this->proto.set_charge_time(0.0f);
  }
}

bool Item::UseOnEntity(RunningState &state, Mob &user, uint32_t id) {
  if (!this->CanUse(state)) return false;

  float charge = this->NeedsChargeUp() ? this->proto.charge_time() : 1.0;

  if (this->properties->canUseEntity) {
    Entity *entity = state.GetEntity(id);
    if (!entity || entity->GetProperties()->nohit || entity->GetProperties()->noItemUse) {
      return this->UseOnNothing(state, user);
    }

    bool result = false;

    uint32_t entityLock = entity->GetLockedID();
    if (entityLock && this->properties->unlockChance > 0.0 && (this->proto.unlock_id() == entityLock || this->proto.unlock_id() == 0) && state.GetRandom().Chance(this->properties->unlockChance)) {
      entity->Unlock();
      if (this->properties->onUnlockBreak) {
        this->DecAmount();
        if (this->GetAmount() > 0) {
          this->proto.set_durability(this->properties->durability);
        }
      }
      state.GetPlayer().AddMessage("You unlock the "+entity->GetName());
      result = true;
    }

    Mob *mob = dynamic_cast<Mob*>(entity);
    if (mob) mob->AddImpulse(user.GetForward() * this->GetKnockback() * charge);

    // replace item
    auto iter = entity->GetProperties()->onUseItemReplace.find(this->properties->name);
    if (iter == entity->GetProperties()->onUseItemReplace.end())
      iter = entity->GetProperties()->onUseItemReplace.find("*");

    if (iter != entity->GetProperties()->onUseItemReplace.end()) {
      *this = Item(iter->second);
      result = true;
    } else if (this->properties->damage != 0.0) {
      HealthInfo healthInfo(Stats::MeleeAttack(user, *entity, *this, state.GetRandom()));

      if (entity->GetProperties()->learnEvade && healthInfo.hitType == HitType::Miss) {
        entity->GetBaseStats().UpgradeSkill("evade");
        // TODO: play item "miss.entity" sound
      } else {
        // TODO: play item "hit.entity" sound
      }

      entity->AddHealth(state, healthInfo);
      std::string effect = this->properties->onHitAddBuff.select(state.GetRandom().Float01());
      entity->AddBuff(state, effect);
      result = true;
    }

    // replace entity
    auto iter2 = entity->GetProperties()->onUseEntityReplace.find(this->properties->name);
    if (iter2 == entity->GetProperties()->onUseEntityReplace.end())
      iter2 = entity->GetProperties()->onUseEntityReplace.find("*");

    if (iter2 != entity->GetProperties()->onUseEntityReplace.end()) {
      Entity *entity2 = Entity::Create(iter2->second);
      if (entity2) {
        entity2->SetPosition(entity->GetPosition());
        state.AddEntity(entity2);
      }
      state.RemoveEntity(entity->GetId());
      result = true;
    }

    this->StartCooldown(state, user);
    return result;
  } else {
    return this->UseOnNothing(state, user);
  }
}

bool Item::UseOnCell(RunningState &state, Mob &user, Cell *cell, Side) {
  if (!this->CanUse(state)) return false;

  float charge = this->NeedsChargeUp() ? this->proto.charge_time() : 1.0;

  if (this->properties->canUseCell) {
    cell->OnUseItem(state, user, *this);

    uint32_t cellLock = cell->GetLockedID();
    if (cellLock && this->properties->unlockChance > 0.0 && (this->proto.unlock_id() == cellLock || this->proto.unlock_id() == 0) && state.GetRandom().Chance(this->properties->unlockChance)) {
      cell->Unlock();
      if (this->properties->onUnlockBreak) {
        this->DecAmount();
        if (this->GetAmount() > 0) {
          this->proto.set_durability(this->properties->durability);
        }
      }
      cell->PlaySound(state, "unlock");
      state.GetPlayer().AddMessage("You unlock it.");
      return true;
    }

    if (this->GetBreakBlockStrength()) {
      if (state.GetRandom().Chance((this->properties->breakBlockStrength * charge) / cell->GetInfo().breakStrength)) {
        cell->GetWorld()->BreakBlock(cell->GetPosition());
        cell->PlaySound(state, "break");
        return true;
      }
    }
    cell->PlaySound(state, "hit");
    // TODO: play item "hit.cell" sound

    this->StartCooldown(state, user);
  } else {
    return this->UseOnNothing(state, user);
  }
  return false;
}

bool Item::UseOnNothing(RunningState &state, Mob &user) {
  if (!this->CanUse(state)) return false;
  if (!this->properties->canUseNothing) return false;

  // TODO: play item "use.nothing" sound
  float charge = this->NeedsChargeUp() ? this->proto.charge_time() : 1.0;

  if (this->properties->spawnProjectile != "") {
    Projectile *proj = new Projectile(this->properties->spawnProjectile);
    proj->SetOwner(user);
    proj->SetForward(user.GetForward());
    proj->SetPosition(user.GetPosition() + Vector3(0,user.GetProperties()->eyeOffset,0));
    state.AddEntity(proj);
    proj->SetVelocity(proj->GetVelocity() * charge);
    proj->AddVelocity(user.GetVelocity());
    this->StartCooldown(state, user);
    return true;
  } else {
    this->StartCooldown(state, user, false);
  }
  return false;
}

void Item::Draw(Gfx &gfx, bool left) {
  gfx.GetView().Push();
  gfx.GetView().Scale(Vector3(left ? 1 : -1, 1, 1));
  gfx.GetView().Translate(Vector3(1, -2, 4));

  gfx.GetView().Rotate(40, Vector3(0, 1, 0));

  gfx.GetView().Translate(Vector3(1,-1,0));
  switch(properties->useMovement) {
    case UseMovement::SlashMovement:  gfx.GetView().Rotate(this->GetCooldownFrac()*60-60, Vector3(0,0,1)); break;
    case UseMovement::StabMovement:   gfx.GetView().Rotate(-50, Vector3(0,0,1));
                                      gfx.GetView().Translate(Vector3(0.5+std::pow(1.0-this->GetCooldownFrac(),3),0,0)); break;
    case UseMovement::RecoilMovement: gfx.GetView().Rotate(-60, Vector3(0,0,1));
                                      gfx.GetView().Translate(Vector3(this->GetCooldownFrac(),0,0)); break;
  }
  gfx.GetView().Translate(Vector3(-1 + this->GetCharge(),1,0));
  gfx.GetView().Scale(Vector3(2,2,2));
  gfx.DrawSprite(this->sprite, Vector3(0,0,0), left, false);
  gfx.GetView().Pop();
}

void Item::DrawIcon(Gfx &gfx, const Point &p) const {
  gfx.DrawIcon(this->sprite, p);

  if (this->properties->durability != this->GetDurability()) {
    float dur = this->GetDurability() / this->properties->durability;
    int frame = 8 * dur;
    gfx.SetTextureFrame(this->durabilityTex, 0, frame, 8);
    gfx.DrawIconQuad(p);
  }

  if (this->GetAmount() > 1)
    RenderString(ToString(this->GetAmount()), "small").Draw(gfx, p - Point(12, 12));

}

void
Item::DrawSprite(Gfx &gfx, const Vector3 &pos) const {
  gfx.DrawSprite(this->sprite, pos);
}

void
Item::ModifyStats(Stats &stats, bool forceEquipped) const {
  float f = 1.0 + 0.75*this->proto.modifier();
  if (this->GetBeatitude() == Beatitude::Blessed) f *=  1.5;

  if (this->IsEquipped() || forceEquipped) {
    stats.SetStrength     (stats.GetStrength()      + this->properties->eqAddStr   * f);
    stats.SetAgility      (stats.GetAgility()       + this->properties->eqAddAgi   * f);
    stats.SetDexterity    (stats.GetDexterity()     + this->properties->eqAddDex   * f);
    stats.SetIntelligence (stats.GetIntelligence()  + this->properties->eqAddInt   * f);
    stats.SetVitality     (stats.GetVitality()      + this->properties->eqAddVit   * f);
    stats.SetLuck         (stats.GetLuck()          + this->properties->eqAddLuk   * f);

    stats.SetAttackBonus       (stats.GetAttackBonus()        + this->properties->eqAddAtk   * f);
    stats.SetDefenseBonus      (stats.GetDefenseBonus()       + this->properties->eqAddDef   * f);
    stats.SetMagicAttackBonus  (stats.GetMagicAttackBonus()   + this->properties->eqAddMAtk  * f);
    stats.SetMagicDefenseBonus (stats.GetMagicDefenseBonus()  + this->properties->eqAddMDef  * f);
    stats.SetMaxHealthBonus    (stats.GetMaxHealthBonus()     + this->properties->eqAddHP    * f);
  } else {
    stats.SetStrength     (stats.GetStrength()      + this->properties->uneqAddStr   * f);
    stats.SetAgility      (stats.GetAgility()       + this->properties->uneqAddAgi   * f);
    stats.SetDexterity    (stats.GetDexterity()     + this->properties->uneqAddDex   * f);
    stats.SetIntelligence (stats.GetIntelligence()  + this->properties->uneqAddInt   * f);
    stats.SetVitality     (stats.GetVitality()      + this->properties->uneqAddVit   * f);
    stats.SetLuck         (stats.GetLuck()          + this->properties->uneqAddLuk   * f);

    stats.SetAttackBonus       (stats.GetAttackBonus()        + this->properties->uneqAddAtk   * f);
    stats.SetDefenseBonus      (stats.GetDefenseBonus()       + this->properties->uneqAddDef   * f);
    stats.SetMagicAttackBonus  (stats.GetMagicAttackBonus()   + this->properties->uneqAddMAtk  * f);
    stats.SetMagicDefenseBonus (stats.GetMagicDefenseBonus()  + this->properties->uneqAddMDef  * f);
    stats.SetMaxHealthBonus    (stats.GetMaxHealthBonus()     + this->properties->uneqAddHP    * f);
  }
  if (this->effect) this->effect->ModifyStats(stats, this->IsEquipped(), this->proto.modifier(), this->GetBeatitude());
}

std::string
Item::GetDisplayName(bool capitalize) const {
  std::string amountString;
  if (this->GetAmount() > 1)
    amountString = u8" \u00d7 " + ToString(this->GetAmount());

  const char *modifierString = "";
  const char *beatitudeString = "";
  const char *nameString = this->properties->unidentifiedName.c_str();
  const char *effectString = "";

  if (this->proto.is_item_identified() || (this->typeIdentified && this->properties->effects.size()==1)) {
    switch(this->proto.modifier()) {
      case -2: modifierString = "terrible "; break;
      case -1: modifierString = "bad "; break;
      case  0: modifierString = ""; break;
      case  1: modifierString = "good "; break;
      case  2: modifierString = "excellent "; break;
    }

    if (this->proto.modifier() < 0) {
      Stats statsA = Stats();
      Stats statsB = Stats();
      ModifyStats(statsA);
      if (statsA == statsB) modifierString = "useless ";
    }
    switch(this->GetBeatitude()) {
      case Beatitude::Blessed: beatitudeString = "blessed"; break;
      case Beatitude::Cursed:  beatitudeString = "cursed";  break;
      default: beatitudeString = "";
    }

    nameString = this->properties->identifiedName.c_str();
  }

  if (this->effect && (this->proto.is_item_identified() || (this->typeIdentified && this->properties->effects.size()==1)) ) {
    effectString = this->effect->displayName.c_str();
  }

  std::string name;
  name += beatitudeString;
  name += modifierString;
  name += nameString;
  name += effectString;
  name += amountString;

  if (capitalize && name != "") name[0] = ::toupper(name[0]);
  return name;
}

Stats
Item::GetDisplayStats() const {
  Stats stats;
  stats.SetMaxHealthBonus(0);
  this->ModifyStats(stats, true);
  return stats;
}

std::shared_ptr<Item>
Item::Combine(const std::shared_ptr<Item> &other) {
  if (this->GetAmount() > 1 || other->GetAmount() > 1) {
    // can only combine single items
    return nullptr;
  }

  std::shared_ptr<Item> result;

  if (this->properties->onCombineEffect != "") {
    const EffectProperties &effect = getEffect(this->properties->onCombineEffect);
    other->proto.set_modifier(other->proto.modifier() + effect.onCombineAddModifier);
    if (effect.onCombineRemoveCurse && other->IsCursed()) {
      other->proto.set_beatitude(int32_t(Beatitude::Normal));
    }
    if (effect.onCombineIdentify) {
      other->proto.set_is_item_identified(true);
    }
    this->proto.set_is_item_identified(true);
    if (this->properties->durability != 0.0) {
      this->proto.set_durability(this->GetDurability() - this->properties->combineDurability);
      if (this->GetDurability() <= 0.0) this->isRemovable = true;
    }
    if (effect.onCombineBreak) {
      // BUG: if item just identified itself this will prevent the game to notice that
      this->isRemovable = true;
    }
    result = other;
  }
  // check combination recipies
  auto iter = this->properties->combinations.find(other->properties->name);
  if (iter != this->properties->combinations.end()) {
    result = other;
    result->ReplaceWith(iter->second);
  }

  return result;
}

std::shared_ptr<Item>
Item::Consume(RunningState &state, Entity &user) {
  if (state.GetPlayer().GetId() == user.GetId() && this->properties->onConsumeVerb != "") {
    state.GetPlayer().AddMessage("You "+this->properties->onConsumeVerb+" the "+this->GetDisplayName()+".");
  }

  if (this->properties->onConsumeEffect != "") {
    const EffectProperties &effect = getEffect(this->properties->onConsumeEffect);
    effect.Consume(state, user);
    user.OnBuffAdded(state, effect);
  }

  if (this->properties->onConsumeAddBuff != "") {
    user.AddBuff(state, this->properties->onConsumeAddBuff);
  }

  if (this->properties->onConsumeTeleport) {
    user.Teleport(state, Vector3(state.GetWorld().GetRandomTeleportTarget(state.GetRandom())));
  }

  state.GetGame().SetIdentified(this->properties->name);

  this->isRemovable = true;

  if (this->properties->onConsumeResult != "") {
    return std::shared_ptr<Item>(new Item(this->properties->onConsumeResult));
  }
  return nullptr;
}

void
Item::SetEquipped(bool equipped) {
  this->proto.set_is_equipped(equipped);
  if (!equipped) {
    this->proto.set_is_charging(false);
  }
}

void
Item::AddAmount(int amt) {
  if (amt > 0) {
    this->SetAmount(this->GetAmount() + amt);
    return;
  }

  if (this->GetAmount() < (size_t)-amt) {
    this->SetAmount(1);
  } else {
    this->SetAmount(this->GetAmount() + amt);
  }
}

bool
Item::CanStack(const Item &other) const {
  return properties == other.properties && 
         properties->stackable &&
         GetDurability() == other.GetDurability() && 
         GetBeatitude()  == other.GetBeatitude() &&
         GetModifier()   == other.GetModifier();
}
/*
Serializer &operator << (Serializer &ser, const Item &item) {
  ser << item.properties->name;
  ser << (item.effect?item.effect->name:"");
  // ser << item.sprite;
  ser << item.cooldownFrac;
  ser << item.durability;
  ser << item.nextUseT;

  ser << (int8_t)item.beatitude << item.modifier;
  ser << item.amount;

  return ser;
}

Deserializer &operator >> (Deserializer &deser, Item *&item) {
  Log("deser item\n");

  std::string type, effect;
  deser >> type >> effect;

  item = new Item(type);
  item->initDone = true;

  item->effect = &getEffect(effect);

  // deser << item->sprite;
  deser >> item->cooldownFrac;
  deser >> item->durability;
  deser >> item->nextUseT;

  deser >> (int8_t&)item->beatitude >> item->modifier;
  deser >> item->amount;

  return deser;
}
*/
