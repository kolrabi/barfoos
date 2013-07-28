#ifndef BARFOOS_EFFECT_H
#define BARFOOS_EFFECT_H

#include "common.h"

#include "stats.h"

#include "properties.h"

struct EffectProperties : public Properties {
  std::string name = "";
  std::string feeling = "";
  std::string displayName = "";
  
  float range = 1.0;
  float damage = 1.0;
  float walkSpeed = 1.0;
  float knockback = 1.0;
  
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
  
  float cooldown = 1.0;
  
  float durability = 1.0;
  float useDurability = 1.0;
  float equipDurability = 1.0;
  
  float breakBlockStrength = 1.0;
  
  bool onCombineRemoveCurse = false;
  bool onCombineIdentify = false;
  int onCombineAddModifier = 0;
  float addHealth = 0;
  
  float duration = 0.0;
  bool extend = false;
  
  Element element = Element::Physical;
  
  virtual void ParseProperty(const std::string &name) override;
  
  void ModifyStats(Stats &stats, bool equipped) const;
  void Consume(RunningState &state, Entity &user) const;
  void Update(RunningState &state, Entity &user) const;
};

void LoadEffects();
const EffectProperties &getEffect(const std::string &name);

#endif
