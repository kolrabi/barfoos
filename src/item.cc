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

#include "serializer.h"
#include "deserializer.h"

Item::Item(const std::string &type) : 
  initDone(false),
  properties(&getItem(type)),
  effect(nullptr),
  durabilityTex(loadTexture("gui/durability")),
  isRemovable(false),
  sprite(this->properties->sprite),
  cooldownFrac(0),
  durability(this->properties->durability),
  isEquipped(false),
  nextUseT(0.0),
  beatitude(Beatitude::Normal),
  modifier(0),
  identified(false),
  amount(1),
  unlockID(0)
{
  if (!this->sprite.animations.empty()) this->sprite.StartAnim(0);
}

Item::~Item() {
}

void Item::ReplaceWith(const std::string &type) {
  this->initDone = false;
  this->properties = &getItem(type);
  this->isRemovable = false;
  this->sprite = this->properties->sprite;
  this->cooldownFrac = 0;
  this->durability = this->properties->durability;
  this->nextUseT = 0.0;
  this->identified = false;
  if (!this->sprite.animations.empty()) this->sprite.StartAnim(0);
}

bool Item::CanUse(RunningState &state) const {
  return (this->durability > 0 || this->properties->durability == 0.0) && 
         this->properties->cooldown >= 0.0 && 
         this->nextUseT < state.GetGame().GetTime();
}
  
void Item::StartCooldown(RunningState &state, Entity &user, bool damage) {
  if (damage) this->durability -= this->properties->useDurability * this->effect->useDurability;
  
  this->nextUseT = state.GetGame().GetTime() + this->GetCooldown() / (1.0 + Const::AttackSpeedFactorPerAGI*user.GetEffectiveStats().agi);
  if (this->properties->onUseIdentify) this->identified = true;
}

void Item::Update(RunningState &state) {
  Game &game = state.GetGame();
  
  if (!this->initDone) {
    this->initDone = true;
    
    if (!this->properties->noModifier) {
      this->modifier = state.GetRandom().Integer(2) + state.GetRandom().Integer(2) - 2;
    }
    
    if (!this->properties->noBeatitude) {
      if (state.GetRandom().Chance(0.01)) {
        this->beatitude = Beatitude::Cursed;
        this->modifier = -2;
      } else if (state.GetRandom().Chance(0.1) && this->modifier == 2) {
        this->beatitude = Beatitude::Blessed;
      }
    }
    
    this->effect = this->properties->effects.size() == 0 ? &getEffect("") : &getEffect(this->properties->effects.select(game.GetRandom().Float01()));
    this->durability *= this->effect->durability;
  }

  if (!this->identified) this->identified = game.IsIdentified(this->properties->name);
  else                   game.SetIdentified(this->properties->name);

  // reduce durability while equipped  
  if (this->isEquipped) { 
    this->sprite.currentAnimation = this->properties->equipAnim;
    this->durability -= this->properties->equipDurability * game.GetDeltaT() * this->effect->equipDurability;
  } else {
    this->sprite.currentAnimation = 0;
  }

  this->sprite.Update(game.GetDeltaT());
  
  // when broken, replace or remove
  if (this->durability <= 0 && this->properties->durability != 0.0) {
    std::string replacement = this->properties->replacement;
    // TODO: message: your x broke
    Log("Your %s broke...\n", this->GetDisplayName().c_str());
    if (replacement != "") {
      this->ReplaceWith(replacement);
      return;
    } else {
      this->isRemovable = true;
    }
  }
  
  // cooldown fraction: 1.0 full cooldown left, 0.0 cooldown over
  cooldownFrac = (nextUseT - game.GetTime())/this->properties->cooldown;
  if (cooldownFrac < 0) cooldownFrac = 0;
}

void Item::UseOnEntity(RunningState &state, Mob &user, uint32_t id) {
  if (!this->CanUse(state)) return;

  if (this->properties->canUseEntity) {
    Entity *entity = state.GetEntity(id);
    if (!entity || entity->GetProperties()->nohit) {
      this->UseOnNothing(state, user);
      return;
    }
    
    uint32_t entityLock = entity->GetLockedID();
    if (entityLock && this->properties->unlockChance > 0.0 && (this->unlockID == entityLock || this->unlockID == 0) && state.GetRandom().Chance(this->properties->unlockChance)) {
      entity->Unlock();
      this->isRemovable = true;
      // TODO: message "unlocked!"
      Log("unlocked!\n");
    }
    
    Mob *mob = dynamic_cast<Mob*>(entity);
    if (mob) mob->AddImpulse(user.GetForward() * this->GetKnockback());

    // replace item
    auto iter = entity->GetProperties()->onUseItemReplace.find(this->properties->name);
    if (iter == entity->GetProperties()->onUseItemReplace.end())
      iter = entity->GetProperties()->onUseItemReplace.find("*");
      
    if (iter != entity->GetProperties()->onUseItemReplace.end()) {
      *this = Item(iter->second);
    } else {
      HealthInfo healthInfo(Stats::MeleeAttack(user, *entity, *this, state.GetRandom()));
      entity->AddHealth(state, healthInfo);
      std::string effect = this->properties->onHitAddBuff.select(state.GetRandom().Float01());
      entity->AddBuff(state, effect);
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
    }
    
    this->StartCooldown(state, user);
  } else {
    this->UseOnNothing(state, user);
  }
}

void Item::UseOnCell(RunningState &state, Mob &user, Cell *cell, Side) {
  if (!this->CanUse(state)) return;
  
  if (this->properties->canUseCell) {
    uint32_t cellLock = cell->GetLockedID();
    if (cellLock && this->properties->unlockChance > 0.0 && (this->unlockID == cellLock || this->unlockID == 0) && state.GetRandom().Chance(this->properties->unlockChance)) {
      cell->Unlock();
      this->isRemovable = true;
      // TODO: message "unlocked!"
      Log("unlocked!\n");
    }
    
    if (this->GetBreakBlockStrength() && state.GetRandom().Chance(this->properties->breakBlockStrength / cell->GetInfo().breakStrength)) {
      cell->GetWorld()->BreakBlock(cell->GetPosition());
    }
    
    this->StartCooldown(state, user);
  } else {
    this->UseOnNothing(state, user);
  }
}

void Item::UseOnNothing(RunningState &state, Mob &user) {
  if (!this->CanUse(state)) return;
  if (!this->properties->canUseNothing) return;
  
  if (this->properties->spawnProjectile != "") {
    Projectile *proj = new Projectile(this->properties->spawnProjectile);
    proj->SetOwner(user);
    proj->SetAngles(user.GetAngles());
    proj->SetPosition(user.GetPosition() + Vector3(0,user.GetProperties()->eyeOffset,0));
    proj->AddVelocity(user.GetVelocity());
    state.AddEntity(proj);
    this->StartCooldown(state, user);
  } else {
    this->StartCooldown(state, user, false);
  }
}

void Item::Draw(Gfx &gfx, bool left) {
  gfx.GetView().Push();
  gfx.GetView().Scale(Vector3(left ? 1 : -1, 1, 1));
  gfx.GetView().Translate(Vector3(1, -2, 4));
  
  gfx.GetView().Rotate(40, Vector3(0, 1, 0));

  gfx.GetView().Translate(Vector3(1,-1,0));
  switch(properties->useMovement) {
    case UseMovement::SlashMovement:  gfx.GetView().Rotate(cooldownFrac*60-60, Vector3(0,0,1)); break;
    case UseMovement::StabMovement:   gfx.GetView().Rotate(-60, Vector3(0,0,1));
                                      gfx.GetView().Translate(Vector3(-cooldownFrac,0,0)); break;
    case UseMovement::RecoilMovement: gfx.GetView().Rotate(-60, Vector3(0,0,1));
                                      gfx.GetView().Translate(Vector3(cooldownFrac,0,0)); break;
  }
  gfx.GetView().Translate(Vector3(-1,1,0));
  gfx.GetView().Scale(Vector3(2,2,2));
  gfx.DrawSprite(this->sprite, Vector3(0,0,0), left, false);
  gfx.GetView().Pop();
}

void Item::DrawIcon(Gfx &gfx, const Point &p) const {
  gfx.DrawIcon(this->sprite, p);
  
  if (this->properties->durability != this->durability) {
    float dur = this->durability / this->properties->durability;
    int frame = 8 * dur;
    gfx.SetTextureFrame(this->durabilityTex, 0, frame, 8);
    gfx.DrawIconQuad(p);
  }

  if (amount > 1)
    RenderString(ToString(amount), "small").Draw(gfx, p - Point(12, 12));
  
}

void
Item::DrawSprite(Gfx &gfx, const Vector3 &pos) const {
  gfx.DrawSprite(this->sprite, pos);
}

void 
Item::ModifyStats(Stats &stats, bool forceEquipped) const {
  if (this->isEquipped || forceEquipped) {
    stats.str += this->properties->eqAddStr   * (1 + 0.5*this->modifier);
    stats.agi += this->properties->eqAddAgi   * (1 + 0.5*this->modifier);
    stats.dex += this->properties->eqAddDex   * (1 + 0.5*this->modifier);
    stats.def += this->properties->eqAddDef   * (1 + 0.5*this->modifier);
    stats.maxHealth += this->properties->eqAddHP * (1 + 0.5*this->modifier);
  } else {
    stats.str += this->properties->uneqAddStr * (1 + 0.5*this->modifier);
    stats.agi += this->properties->uneqAddAgi * (1 + 0.5*this->modifier);
    stats.dex += this->properties->uneqAddDex * (1 + 0.5*this->modifier);
    stats.def += this->properties->uneqAddDef * (1 + 0.5*this->modifier);
    stats.maxHealth += this->properties->uneqAddHP * (1 + 0.5*this->modifier);
  }
  if (this->effect) this->effect->ModifyStats(stats, this->isEquipped);
}

std::string
Item::GetDisplayName(bool capitalize) const {
  std::string amountString;
  if (this->amount > 1)
    amountString = u8" \u00d7 " + ToString(this->amount);
    
  if (!this->identified) return this->properties->unidentifiedName + amountString;

  const char *modifierString = "";
  
  switch(this->modifier) {
    case -2: modifierString = "terrible "; break;
    case -1: modifierString = "bad "; break;
    case  0: modifierString = ""; break;
    case  1: modifierString = "good "; break;
    case  2: modifierString = "excellent "; break;
  }

  if (this->modifier < 0) {  
    Stats statsA = Stats();
    Stats statsB = Stats();
    ModifyStats(statsA);
    if (statsA == statsB) modifierString = "useless ";
  }
  
  char tmp[1024];
  snprintf(tmp, sizeof(tmp), "%s%s%s", this->beatitude == Beatitude::Normal ? "" : (this->beatitude == Beatitude::Blessed ? "blessed " : "cursed "), modifierString, this->properties->identifiedName.c_str());
  
  std::string name = tmp;
  if (this->effect) { name += this->effect->name; }

  if (capitalize && name != "") name[0] = ::toupper(name[0]);
  
  return name;
}

Stats
Item::GetDisplayStats() const {
  Stats stats;
  stats.str = 0; stats.agi = 0; stats.dex = 0; stats.def = 0;
  stats.maxHealth = 0;
  this->ModifyStats(stats, true);
  return stats;
}

std::shared_ptr<Item> 
Item::Combine(const std::shared_ptr<Item> &other) {
  if (this->amount > 1 || other->amount > 1) {
    // can only combine single items
    return nullptr;
  }

  // TODO: check combination recipies
  
  if (this->properties->onCombineEffect != "") {
    const EffectProperties &effect = getEffect(this->properties->onCombineEffect);
    other->modifier += effect.onCombineAddModifier;
    if (effect.onCombineRemoveCurse && other->IsCursed()) other->beatitude = Beatitude::Normal;
    if (effect.onCombineIdentify) {
      other->identified = true;
    }
    this->identified = true;
    this->isRemovable = true;
    return other;
  }
  
  return nullptr;
}

std::shared_ptr<Item> 
Item::Consume(RunningState &state, Entity &user) {
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
  this->isEquipped = equipped; 
}

void
Item::AddAmount(int amt) {
  if (amt > 0) {
    this->amount += amt;
    return;
  }
  
  if (this->amount < (size_t)-amt) {
    this->amount = 1; 
  } else {
    this->amount += amt;
  }
}

bool 
Item::CanStack(const Item &other) const {
  return properties == other.properties && properties->stackable && 
         durability == other.durability && beatitude == other.beatitude && 
         modifier == other.modifier;
}
  
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

