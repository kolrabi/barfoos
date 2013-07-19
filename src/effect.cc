#include "effect.h"
#include "entity.h"
#include "world.h"
#include "mob.h"
#include "game.h"
#include "gfx.h"
#include "projectile.h"
#include "texture.h"

#include <unordered_map>

static std::unordered_map<std::string, EffectProperties> allEffects;
EffectProperties defaultEffect;

const EffectProperties &getEffect(const std::string &name) {
  if (name == "") return defaultEffect;
  if (allEffects.find(name) == allEffects.end()) {
    Log("Properties for effect of type '%s' not found\n", name.c_str());
    return defaultEffect;
  }
  return allEffects[name];
}

void
EffectProperties::ParseProperty(const std::string &cmd) {
  if (cmd == "damage") {
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
    
  } else if (cmd == "durability") {
    Parse(this->durability);
  } else if (cmd == "usedurability") {
    Parse(this->useDurability);
  } else if (cmd == "equipdurability") {
    Parse(this->equipDurability);
    
  } else if (cmd == "breakblockstrength") {
    Parse(this->breakBlockStrength);

  } else if (cmd == "oncombineremovecurse") {
    this->onCombineRemoveCurse = true;
  } else if (cmd == "oncombineidentify") {
    this->onCombineIdentify = true;
  } else if (cmd == "oncombineaddmodifier") {
    Parse(this->onCombineAddModifier);
    
  } else if (cmd == "name") {
    Parse(this->name);
    
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
      fclose(f);
    }
  }
}

void 
EffectProperties::ModifyStats(Stats &stats, bool forceEquipped) const {
  if (forceEquipped) {
    stats.str += this->eqAddStr;
    stats.agi += this->eqAddAgi;
    stats.dex += this->eqAddDex;
    stats.def += this->eqAddDef;
    stats.maxHealth = this->eqAddHP;
  } else {
    stats.str += this->uneqAddStr;
    stats.agi += this->uneqAddAgi;
    stats.dex += this->uneqAddDex;
    stats.def += this->uneqAddDef;
    stats.maxHealth = this->uneqAddHP;
  }
}
