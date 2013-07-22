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
  info.element = item.GetElement();

  // TODO: buffs
  
  // get stats
  Stats atkStat = attacker.GetEffectiveStats();
  Stats defStat = victim.GetEffectiveStats();
  
  // determine hit
  int dex = random.Integer(std::max(atkStat.dex, 1));
  int agi = random.Integer(std::max(defStat.agi, 1));

  HitType hit = (dex < agi) ? HitType::Miss : HitType::Normal;
  if (random.Chance(atkStat.dex*0.001)) hit = HitType::Critical;

  info.hitType = hit;
  
  int dmg = atkStat.str * 0.1 * item.GetDamage();
  if (dmg < 1) dmg = 1;

  int amount = dmg - defStat.def * 0.1;
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

HealthInfo 
Stats::ExplosionAttack(const Entity &attacker, const Entity &victim, float damage, Element element) {
  HealthInfo info;
  info.dealerId = attacker.GetId();
  info.type = HealthType::Explosion;
  info.element = element;
  
  // TODO: buffs
  
  // get stats
  Stats defStat = victim.GetEffectiveStats();
  
  info.amount = -(damage - defStat.def * 0.1);
  bool kill = -info.amount > victim.GetHealth();
  float expDmg = kill ? victim.GetHealth() : -info.amount;
  info.exp = (expDmg / defStat.maxHealth) * 0.5 * victim.GetProperties()->exp + kill ? victim.GetProperties()->exp : 0;

  return info;
}

size_t 
Stats::GetLevel() {
  return std::log(this->exp + 1) / std::log(1.7) + 1;
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
  return std::pow(1.7, lvl - 1);
}

std::string
Stats::GetToolTip() const {
  std::string tooltip;
  char tmp[256];
  
  if (str) {
    snprintf(tmp, sizeof(tmp), "str: %+d\n", str);
    tooltip += tmp;
  }

  if (agi) {
    snprintf(tmp, sizeof(tmp), "agi: %+d\n", agi);
    tooltip += tmp;
  }

  if (dex) {
    snprintf(tmp, sizeof(tmp), "dex: %+d\n", dex);
    tooltip += tmp;
  }
  
  if (def) {
    snprintf(tmp, sizeof(tmp), "def: %+d\n", def);
    tooltip += tmp;
  }
  
  if (maxHealth) {
    snprintf(tmp, sizeof(tmp), "hp:  %+d\n", maxHealth);
    tooltip += tmp;
  }
  return tooltip;
}
