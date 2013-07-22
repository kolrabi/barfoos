#include "item.h"
#include "entity.h"
#include "world.h"
#include "mob.h"
#include "game.h"
#include "gfx.h"
#include "projectile.h"
#include "texture.h"
#include "text.h"

#include <unordered_map>
#include <algorithm>

static std::unordered_map<std::string, ItemProperties> allItems;
ItemProperties defaultItem;

const ItemProperties &getItem(const std::string &name) {
  if (allItems.find(name) == allItems.end()) {
    Log("Properties for entity of type '%s' not found\n", name.c_str());
    return defaultItem;
  }
  return allItems[name];
}

static void shufflePotions(Game &game) {
  std::vector<std::string> potionsBefore;
  std::vector<std::string> potionsAfter, tmp;
  for (auto &i : allItems) {
    if (i.second.isPotion) {
      potionsBefore.push_back(i.first);
      tmp.push_back(i.first);
    }
  }
  
  while(tmp.size() > 0) {
    size_t i = game.GetRandom().Integer(tmp.size());
    potionsAfter.push_back(tmp[i]);
    tmp.erase(tmp.begin() + i);
  }
  
  for (size_t i = 0; i < potionsBefore.size(); i++) {
    // swap appearances and descriptions
    std::swap(allItems[potionsBefore[i]].sprite,           allItems[potionsAfter[i]].sprite);
    std::swap(allItems[potionsBefore[i]].unidentifiedName, allItems[potionsAfter[i]].unidentifiedName);
  }
}

void
ItemProperties::ParseProperty(const std::string &cmd) {
  if (cmd == "tex") {
    Parse("items/texture/", this->sprite.texture);
  } else if (cmd == "frames") {
    Parse(this->sprite.totalFrames);
  } else if (cmd == "anim") {
    size_t firstFrame = 0;
    size_t frameCount = 0;
    float  fps = 0;
    
    Parse(firstFrame);
    Parse(frameCount);
    Parse(fps);
    
    this->sprite.animations.push_back(Animation(firstFrame, frameCount, fps));
    if (this->sprite.animations.size() == 1) this->sprite.StartAnim(0);
    
  } else if (cmd == "size") {
    Parse(this->sprite.width);
    Parse(this->sprite.height);
  } else if (cmd == "damage") {
    Parse(this->damage);
    
  } else if (cmd == "eqstr") {
    Parse(this->eqAddStr);
  } else if (cmd == "eqdex") {
    Parse(this->eqAddDex);
  } else if (cmd == "eqagi") {
    Parse(this->eqAddAgi);
  } else if (cmd == "eqdef") {
    Parse(this->eqAddDef);
  } else if (cmd == "eqhp") {
    Parse(this->eqAddHP);
    
  } else if (cmd == "uneqstr") {
    Parse(this->uneqAddStr);
  } else if (cmd == "uneqdex") {
    Parse(this->uneqAddDex);
  } else if (cmd == "uneqagi") {
    Parse(this->uneqAddAgi);
  } else if (cmd == "uneqdef") {
    Parse(this->uneqAddDef);
  } else if (cmd == "uneqhp") {
    Parse(this->uneqAddHP);
    
  } else if (cmd == "cooldown") {
    Parse(this->cooldown);
  } else if (cmd == "range") {
    Parse(this->range);
  } else if (cmd == "light") {
    Parse(this->light);
    
  } else if (cmd == "flicker") {
    this->flicker = true;
  } else if (cmd == "canusecell") {
    this->canUseCell = true;
  } else if (cmd == "canuseentity") {
    this->canUseEntity = true;
  } else if (cmd == "canusenothing") {
    this->canUseNothing = true;
  } else if (cmd == "nomodifier") {
    this->noModifier = true;
  } else if (cmd == "nobeatitude") {
    this->noBeatitude = true;
    
  } else if (cmd == "hand") {
    this->equippable |= (1<<(size_t)InventorySlot::LeftHand) | (1<<(size_t)InventorySlot::RightHand);
  } else if (cmd == "helmet") {
    this->equippable |= (1<<(size_t)InventorySlot::Helmet);
  } else if (cmd == "armor") {
    this->equippable |= (1<<(size_t)InventorySlot::Armor);
  } else if (cmd == "leftring") {
    this->equippable |= (1<<(size_t)InventorySlot::LeftRing);
  } else if (cmd == "rightring") {
    this->equippable |= (1<<(size_t)InventorySlot::RightRing);
  } else if (cmd == "lefthand") {
    this->equippable |= (1<<(size_t)InventorySlot::LeftHand);
  } else if (cmd == "righthand") {
    this->equippable |= (1<<(size_t)InventorySlot::RightHand);
  } else if (cmd == "greaves") {
    this->equippable |= (1<<(size_t)InventorySlot::Greaves);
  } else if (cmd == "boots") {
    this->equippable |= (1<<(size_t)InventorySlot::Boots);

  } else if (cmd == "stack") {
    this->stackable = true;
    
  } else if (cmd == "durability") {
    Parse(this->durability);
  } else if (cmd == "usedurability") {
    Parse(this->useDurability);
  } else if (cmd == "equipdurability") {
    Parse(this->equipDurability);
  } else if (cmd == "equipanim") {
    Parse(this->equipAnim);
    
  } else if (cmd == "replacement") {
    Parse(this->replacement);
  } else if (cmd == "onusespawnprojectile") {
  
    Parse(this->spawnProjectile);
  } else if (cmd == "breakblockstrength") {
    Parse(this->breakBlockStrength);

  } else if (cmd == "identifiedname") {
    Parse(this->identifiedName);
  } else if (cmd == "unidentifiedname") {
    Parse(this->unidentifiedName);
    
  } else if (cmd == "scroll") {
    this->unidentifiedName = this->game->GetScrollName();
  } else if (cmd == "potion") {
    this->isPotion = true;
    
  } else if (cmd == "oncombineeffect") {
    Parse(this->onCombineEffect);
  } else if (cmd == "onconsumeeffect") {
    Parse(this->onConsumeEffect);
  } else if (cmd == "onconsumeresult") {
    Parse(this->onConsumeResult);
  } else if (cmd == "onconsumeaddbuff") {
    Parse(this->onConsumeAddBuff);

  } else if (cmd == "effect") {
    float w;
    Parse(w);
    
    std::string name;
    Parse(name);
    
    this->effects[name] = w;
    
  } else if (cmd != "") {
    this->SetError("ignoring '" + cmd + "'");
  }
}

void
LoadItems(Game &game) {
  allItems.clear();
  
  std::vector<std::string> assets = findAssets("items");
  for (const std::string &name : assets) {
    FILE *f = openAsset("items/"+name);
    if (f) {
      allItems[name].game = &game;
      allItems[name].name = name;
      allItems[name].identifiedName = name;
      allItems[name].unidentifiedName = name;
      allItems[name].ParseFile(f);
      fclose(f);
    }
  }
  
  shufflePotions(game);
}

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
  amount(1)
{
  if (this->sprite.animations.size() > 0) this->sprite.StartAnim(0);
}

Item::~Item() {
}

bool Item::CanUse(Game &game) const {
  return (this->durability > 0 || this->properties->durability == 0.0) && 
         this->properties->cooldown >= 0.0 && 
         this->nextUseT < game.GetTime();
}
  
void Item::StartCooldown(Game &game, Entity &user) {
  this->durability -= this->properties->useDurability * this->effect->useDurability;
  this->nextUseT = game.GetTime() + this->GetCooldown() / (1.0 + Const::AttackSpeedFactorPerAGI*user.GetEffectiveStats().agi);
}

void Item::Update(Game &game) {
  if (!this->initDone) {
    this->initDone = true;
    
    if (!this->properties->noModifier) {
      this->modifier = game.GetRandom().Integer(2) + game.GetRandom().Integer(2) - 2;
    }
    
    if (!this->properties->noBeatitude) {
      if (game.GetRandom().Chance(0.01)) {
        this->beatitude = Beatitude::Cursed;
        this->modifier = -2;
      } else if (game.GetRandom().Chance(0.1) && this->modifier == 2) {
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
    if (replacement != "") {
      bool isEquipped = this->isEquipped;
      *this = Item(replacement);
      this->isEquipped = isEquipped;
      return;
    } else {
      this->isRemovable = true;
    }
  }
  
  // cooldown fraction: 1.0 full cooldown left, 0.0 cooldown over
  cooldownFrac = (nextUseT - game.GetTime())/this->properties->cooldown;
  if (cooldownFrac < 0) cooldownFrac = 0;
}

void Item::UseOnEntity(Game &game, Mob &user, size_t id) {
  if (!this->CanUse(game)) return;

  if (this->properties->canUseEntity) {
    Entity *entity = game.GetEntity(id);
    if (!entity || entity->GetProperties()->nohit) {
      this->UseOnNothing(game, user);
      return;
    }
    
    auto iter = entity->GetProperties()->onUseItemReplace.find(this->properties->name);
    if (iter != entity->GetProperties()->onUseItemReplace.end()) {
      *this = Item(iter->second);
    } else {
      HealthInfo healthInfo(Stats::MeleeAttack(user, *entity, *this, game.GetRandom()));
      entity->AddHealth(game, healthInfo);
    }
    
    this->StartCooldown(game, user);
  } else {
    this->UseOnNothing(game, user);
  }
}

void Item::UseOnCell(Game &game, Mob &user, Cell *cell, Side side) {
  (void)side;
  
  if (!this->CanUse(game)) return;
  
  if (this->properties->canUseCell) {
    if (this->GetBreakBlockStrength() && game.GetRandom().Chance(this->properties->breakBlockStrength / cell->GetInfo().breakStrength)) {
      cell->GetWorld()->BreakBlock(game, cell->GetPosition());
    }
    this->StartCooldown(game, user);
  } else {
    this->UseOnNothing(game, user);
  }
}

void Item::UseOnNothing(Game &game, Mob &user) {
  if (!this->CanUse(game)) return;
  this->StartCooldown(game, user);
  
  if (this->properties->spawnProjectile != "") {
    Projectile *proj = new Projectile(this->properties->spawnProjectile);
    proj->SetOwner(user);
    proj->SetAngles(user.GetAngles());
    proj->SetPosition(user.GetPosition() + Vector3(0,user.GetProperties()->eyeOffset,0));
    proj->AddVelocity(user.GetVelocity());
    game.AddEntity(proj);
  }
}

void Item::Draw(Gfx &gfx, bool left) {
  gfx.GetView().Push();
  gfx.SetBackfaceCulling(false);
  gfx.GetView().Scale(Vector3(left ? 1 : -1, 1, 1));
  gfx.GetView().Translate(Vector3(1, -2, 4));
  
  gfx.GetView().Rotate(40, Vector3(0, 1, 0));

  gfx.GetView().Translate(Vector3(1,-1,0));
  gfx.GetView().Rotate(cooldownFrac*60-60, Vector3(0,0,1));
  gfx.GetView().Translate(Vector3(-1,1,0));
  gfx.GetView().Scale(Vector3(2,2,2));
  gfx.DrawSprite(this->sprite, Vector3(0,0,0), false);
  gfx.SetBackfaceCulling(true);
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
    stats.str += this->properties->eqAddStr   * (1 + 0.4*this->modifier);
    stats.agi += this->properties->eqAddAgi   * (1 + 0.4*this->modifier);
    stats.dex += this->properties->eqAddDex   * (1 + 0.4*this->modifier);
    stats.def += this->properties->eqAddDef   * (1 + 0.4*this->modifier);
    stats.maxHealth += this->properties->eqAddHP * (1 + 0.4*this->modifier);
  } else {
    stats.str += this->properties->uneqAddStr * (1 + 0.4*this->modifier);
    stats.agi += this->properties->uneqAddAgi * (1 + 0.4*this->modifier);
    stats.dex += this->properties->uneqAddDex * (1 + 0.4*this->modifier);
    stats.def += this->properties->uneqAddDef * (1 + 0.4*this->modifier);
    stats.maxHealth += this->properties->uneqAddHP * (1 + 0.4*this->modifier);
  }
  if (this->effect) this->effect->ModifyStats(stats, this->isEquipped);
}

std::string
Item::GetDisplayName() const {
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
  
  char tmp[1024];
  snprintf(tmp, sizeof(tmp), "%s%s%s", this->beatitude == Beatitude::Normal ? "" : (this->beatitude == Beatitude::Blessed ? "blessed " : "cursed "), modifierString, this->properties->identifiedName.c_str());
  
  std::string name = tmp;
  if (this->effect) { name += this->effect->name; }
  
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
  if (other->properties == this->properties && this->properties->stackable) {
    other->amount += this->amount;
    this->isRemovable = true;
    return other;
  } else if (this->amount > 1 || other->amount > 1) {
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
Item::Consume(Game &game, Entity &user) {
  if (this->properties->onConsumeEffect != "") {
    const EffectProperties &effect = getEffect(this->properties->onConsumeEffect);
    effect.Consume(game, user);
  }
  
  if (this->properties->onConsumeAddBuff != "") {
    user.AddBuff(game, this->properties->onConsumeAddBuff);
  }
  
  game.SetIdentified(this->properties->name);
  
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
  