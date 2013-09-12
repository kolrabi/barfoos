#include "common.h"

#include "spell.h"

#include "entity.h"
#include "projectile.h"
#include "runningstate.h"
#include "fileio.h"

#include <unordered_map>

static std::unordered_map<std::string, Spell> allSpells;
static Spell defaultSpell;

const Spell &getSpell(const std::string &name) {
  if (name == "") return defaultSpell;
  if (allSpells.find(name) == allSpells.end()) {
    Log("Properties for Spell of type '%s' not found\n", name.c_str());
    return defaultSpell;
  }
  return allSpells[name];
}

void
Spell::ParseProperty(const std::string &cmd) {
  if (cmd == "name")                      Parse(this->displayName);
  else if (cmd == "incantation")          Parse(this->incantation);
  else if (cmd == "oncastaddbuffcaster")  Parse(this->onCastAddBuffCaster);
  else if (cmd == "oncastaddbuffaoe")     Parse(this->onCastAddBuffAOE);
  else if (cmd == "oncastaddbufftarget")  Parse(this->onCastAddBuffTarget);

  else if (cmd == "oncastdamagecaster")   Parse(this->onCastDamageCaster);
  else if (cmd == "oncastdamageaoe")      Parse(this->onCastDamageAOE);
  else if (cmd == "oncastdamagetarget")   Parse(this->onCastDamageTarget);

  else if (cmd == "oncastspawnentity")    Parse(this->onCastSpawnEntity);

  else if (cmd == "castaoerange")         Parse(this->castAOERange);
  else if (cmd == "castrange")            Parse(this->castRange);

  else if (cmd == "element")              Parse(this->element);
  else if (cmd == "learnchance")          Parse(this->learnChance);
  
  else if (cmd == "targetself")           this->targetSelf = true;
  else if (cmd == "castinterval")         Parse(this->castInterval);
  else if (cmd == "maxduration")          Parse(this->maxDuration);

  else if (cmd == "castsound")            Parse(this->castSound);
// TODO: particle effects: aoe, line of sight
  else if (cmd != "") {
    this->SetError("ignoring '" + cmd + "'");
  }
}

void
LoadSpells() {
  allSpells.clear();

  std::vector<std::string> assets = findAssets("spells");
  for (const std::string &name : assets) {
    FILE *f = openAsset("spells/"+name);
    if (f) {
      Log("loading spell %s\n", name.c_str());
      allSpells[name].name = name;
      allSpells[name].ParseFile(f);
      fclose(f);
    }
  }
}

const Spell &getSpell(const std::vector<Element> &incantation) {
  for (auto &s:allSpells) {
    if (incantation.size() != s.second.incantation.size()) continue;
    size_t i;
    for (i=0; i<incantation.size(); i++) {
      if (incantation[i] != s.second.incantation[i]) break;
    }
    if  (i == incantation.size()) return s.second;
  }
  return defaultSpell;
}

bool
Spell::Cast(RunningState &state, Mob &user) const {
  if (this->name == "") return false;

  if (!user.HasLearntSpell(this->name)) {
    if (state.GetRandom().Chance(this->learnChance)) {
      user.LearnSpell(this->name);
    } else {
      // TODO: play failed sound
      return false;
    }
  }

  Stats stats = user.GetEffectiveStats();

  // find target
  ID targetId;
  Side selectedCellSide;
  Cell *cell = user.GetSelection(state, this->castRange, nullptr, selectedCellSide, targetId);
  Entity *target = cell ? nullptr : state.GetEntity(targetId);
  
  if (!target && this->targetSelf) target = &user;

  if (this->onCastAddBuffCaster != "") user.AddBuff(state, this->onCastAddBuffCaster);
  if (this->onCastAddBuffAOE    != "" || this->onCastDamageAOE) {
    std::vector<ID> entIDs = state.FindEntities(user.GetPosition(), this->castAOERange);

    if (this->onCastAddBuffAOE != "") {
      for (ID e:entIDs) {
        Entity *ent = state.GetEntity(e);
        if (!ent) continue;
        ent->AddBuff(state, this->onCastAddBuffAOE);
      }
    }

    if (onCastDamageAOE) {
      float damage = this->onCastDamageAOE * (stats.GetMagicAttack() + stats.GetSkill("magic"));
      for (ID e:entIDs) {
        Entity *ent = state.GetEntity(e);
        if (!ent) continue;
        ent->AddHealth(state, Stats::MagicAttack(user.GetId(), *ent, damage, this->element));
      }
    }
  }

  if (this->onCastDamageCaster) {
    float damage = this->onCastDamageCaster * (stats.GetMagicAttack() + stats.GetSkill("magic"));
    user.AddHealth(state, Stats::MagicAttack(user.GetId(), user, damage, this->element));
  }

  if (target) {
    if (this->onCastAddBuffTarget != "") {
      target->AddBuff(state, this->onCastAddBuffTarget);
    }
    if (this->onCastDamageTarget) {
      float damage = this->onCastDamageTarget * (stats.GetMagicAttack() + stats.GetSkill("magic"));
      target->AddHealth(state, Stats::MagicAttack(user.GetId(), *target, damage, this->element));
    }
  }

  if (this->onCastSpawnEntity != "") {
    Entity *entity = Entity::Create(this->onCastSpawnEntity);
    entity->SetOwner(user);
    entity->SetForward(user.GetForward());
    entity->SetPosition(user.GetEyePosition() + user.GetForward());
    state.AddEntity(entity);
    Projectile *proj = dynamic_cast<Projectile*>(entity);
    if (proj) {
      proj->SetVelocity(user.GetForward() * proj->GetProperties()->maxSpeed);
    }
  }

  // TODO: play sound
  return true;
}
