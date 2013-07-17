#include "stats.h"
#include "item.h"
#include "entity.h"
#include "random.h"

HealthInfo 
Stats::MeleeAttack(const Entity &attacker, const Entity &victim, const Item &item, Random &random) {
  HealthInfo info;
  info.dealerId = attacker.GetId();
  info.skill = item.GetProperties().weaponClass;
  info.type = HealthType::Melee;
  
  // TODO: info.element
  // TODO: buffs
  
  // get stats
  Stats atkStat = attacker.GetEffectiveStats();
  Stats defStat = victim.GetEffectiveStats();
  
  // determine hit
  HitType hit = (atkStat.dex - defStat.agi) < 0 ? HitType::Miss : (random.Chance(atkStat.dex*0.01) ? HitType::Critical : HitType::Miss);
  
  float dmg = atkStat.str * 0.1 * item.GetProperties().damage;
  float amount = dmg - defStat.def * 0.1;
  if (amount < 1) amount = 1;
  
  // determine damage
  switch(hit) {
    case HitType::Miss:     info.amount = 0.0; break;
    case HitType::Critical: info.amount = -dmg; break;
    case HitType::Normal:   info.amount = -amount; break;
    default: break;
  }
  
  bool kill = -info.amount > victim.GetHealth();
  float expDmg = kill ? victim.GetHealth() : -info.amount;
  info.exp = (expDmg / defStat.maxHealth) * 0.5 * victim.GetProperties()->exp + kill ? victim.GetProperties()->exp : 0;
  
  return info;
}

size_t 
Stats::GetLevel() {
  return std::log(this->exp + 1) / std::log(1.7);
}

bool
Stats::AddExp(float exp) {
  size_t lvl = this->GetLevel();
  this->exp += exp;
  if (this->GetLevel() != lvl) {
    this->sp ++;
    return true;
  }
  return false;
}

float 
Stats::GetExpForLevel(size_t lvl) {
  return std::pow(1.7, lvl);
}

