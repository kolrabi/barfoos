#include "common.h"

#include "effect.h"

#include "entity.h"
#include "world.h"
#include "runningstate.h"
#include "gfx.h"
#include "texture.h"
#include "item.h"
#include "fileio.h"

#include <unordered_map>

static std::unordered_map<std::string, EffectProperties> allEffects;
static std::unordered_map<std::string, std::vector<std::string>> allEffectGroups;
static EffectProperties defaultEffect;

const EffectProperties &getEffect(const std::string &name) {
  if (name == "") return defaultEffect;
  if (allEffects.find(name) == allEffects.end()) {
    Log("Properties for effect of type '%s' not found\n", name.c_str());
    return defaultEffect;
  }
  return allEffects[name];
}

const std::vector<std::string> &getEffectsInGroup(const std::string &group) {
  return allEffectGroups[group];
}

void
EffectProperties::ParseProperty(const std::string &cmd) {
  if (cmd == "damage") {
    Parse(this->damage);

  } else if (cmd == "feeling") {
    Parse(this->feeling);

  } else if (cmd == "eqstr") {
    Parse(this->eqAddStr);
  } else if (cmd == "eqdex") {
    Parse(this->eqAddDex);
  } else if (cmd == "eqagi") {
    Parse(this->eqAddAgi);
  } else if (cmd == "eqdef") {
    Parse(this->eqAddDef);
  } else if (cmd == "eqmdef") {
    Parse(this->eqAddMDef);
  } else if (cmd == "eqmatk") {
    Parse(this->eqAddMDef);
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
  } else if (cmd == "uneqmdef") {
    Parse(this->uneqAddMDef);
  } else if (cmd == "uneqmatk") {
    Parse(this->uneqAddMDef);
  } else if (cmd == "uneqhp") {
    Parse(this->uneqAddHP);

  } else if (cmd == "cooldown") {
    Parse(this->cooldown);
  } else if (cmd == "range") {
    Parse(this->range);
  } else if (cmd == "walkspeed") {
    Parse(this->walkSpeed);
  } else if (cmd == "knockback") {
    Parse(this->knockback);

  } else if (cmd == "durability") {
    Parse(this->durability);
  } else if (cmd == "usedurability") {
    Parse(this->useDurability);
  } else if (cmd == "equipdurability") {
    Parse(this->equipDurability);

  } else if (cmd == "duration") {
    Parse(this->duration);
  } else if (cmd == "extend") {
    this->extend = true;

  } else if (cmd == "breakblockstrength") {
    Parse(this->breakBlockStrength);

  } else if (cmd == "oncombineremovecurse") {
    this->onCombineRemoveCurse = true;
  } else if (cmd == "oncombineidentify") {
    this->onCombineIdentify = true;
  } else if (cmd == "oncombineaddmodifier") {
    Parse(this->onCombineAddModifier);
  } else if (cmd == "oncombinebreak") {
    this->onCombineBreak = true;
  } else if (cmd == "addhealth") {
    Parse(this->addHealth);

  } else if (cmd == "name") {
    Parse(this->displayName);

  } else if (cmd == "group") {
    Parse(this->groups);

  } else if (cmd == "light") {
    Parse(this->light);
  } else if (cmd != "") {
    this->SetError("ignoring '" + cmd + "'");
  }
}

void
LoadEffects() {
  allEffects.clear();

  std::vector<std::string> assets = findAssets("effects");
  for (const std::string &name : assets) {
    FILE *f = openAsset("effects/"+name);
    if (f) {
      allEffects[name].name = name;
      allEffects[name].ParseFile(f);
      for (auto &g : allEffects[name].groups) {
        allEffectGroups[g].push_back(name);
      }
      fclose(f);
    }
  }
}

void
EffectProperties::ModifyStats(Stats &stats, bool forceEquipped, int modifier, Beatitude beatitude) const {
  float f = 1.0 + 0.75*modifier;
  if (beatitude == Beatitude::Blessed) f *=  1.5;

  if (forceEquipped) {
    stats.str += this->eqAddStr * f;
    stats.agi += this->eqAddAgi * f;
    stats.dex += this->eqAddDex * f;
    stats.def += this->eqAddDef * f;
    stats.mdef += this->eqAddMDef * f;
    stats.matk += this->eqAddMAtk * f;
    stats.maxHealth += this->eqAddHP * f;

    if (beatitude == Beatitude::Normal) {
      stats.walkSpeed *= std::pow(this->walkSpeed, modifier*0.25) * this->walkSpeed;
      stats.cooldown  *= std::pow(this->cooldown,  modifier*0.25) * this->cooldown;
    } else {
      stats.walkSpeed *= this->walkSpeed * this->walkSpeed;
      stats.cooldown  *= this->cooldown  * this->cooldown;
    }
  } else {
    stats.str += this->uneqAddStr * f;
    stats.agi += this->uneqAddAgi * f;
    stats.dex += this->uneqAddDex * f;
    stats.def += this->uneqAddDef * f;
    stats.mdef += this->uneqAddMDef * f;
    stats.matk += this->uneqAddMAtk * f;
    stats.maxHealth += this->uneqAddHP * f;
  }
}

void
EffectProperties::Consume(RunningState &state, Entity &user) const {
  this->ModifyStats(user.GetBaseStats(), true, 0, Beatitude::Normal);
  if (this->addHealth) {
    HealthInfo info(this->addHealth);
    user.AddHealth(state, info);
  }
}

void
EffectProperties::Update(RunningState &state, Entity &owner) const {
  if (this->addHealth) {
    HealthInfo info(this->addHealth * state.GetGame().GetDeltaT());
    owner.AddHealth(state, info);
  }
}
