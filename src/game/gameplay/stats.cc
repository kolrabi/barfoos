#include "common.h"

#include "game/entities/entity.h"
#include "game/gameplay/stats.h"
#include "game/items/item.h"
#include "math/random.h"

/** Calculate the outcome of a melee attack with an item.
 * @param attacker Entity that attacks
 * @param victim Entity that defends
 * @param item Item that is used to attack
 * @param random A random number generator for variance
 * @return Result of the attack.
 */
HealthInfo
Stats::MeleeAttack(const Entity &attacker, const Entity &victim, const Item &item, Random &random) {
  HealthInfo info;
  info.dealerId = attacker.GetId();
  info.skill    = item.GetProperties().useSkill;
  info.type     = HealthType::Melee;
  info.element  = item.GetElement();

  float charge = item.NeedsChargeUp() ? item.GetCharge() : 1.0;

  // get stats
  Stats atkStat = attacker.GetEffectiveStats();
  Stats defStat = victim.GetEffectiveStats();

  uint32_t hitLevel  = atkStat.GetSkill(info.skill) + atkStat.GetHit();
  uint32_t fleeLevel = defStat.GetFlee();

  // determine hit
  int hit  = random.Integer(std::max(hitLevel,  (uint32_t)1));
  int flee = random.Integer(std::max(fleeLevel, (uint32_t)1));

  HitType hitType = (hit < flee) ? HitType::Miss : HitType::Normal;
  if (random.Chance(atkStat.GetCrit()*0.01)) hitType = HitType::Critical;

  info.hitType = hitType;

  int dmg = (atkStat.GetAttack() + item.GetDamage()) * charge;
  if (dmg < 1) dmg = 1;

  int amount = dmg - defStat.GetDefense();
  if (amount < 1) amount = 1;

  // determine damage
  switch(hitType) {
    case HitType::Miss:     info.amount = 0.0; break;
    case HitType::Critical: info.amount = -dmg; break;
    case HitType::Normal:   info.amount = -amount; break;
    default: break;
  }

  bool kill = -info.amount > victim.GetHealth();
  float expDmg = kill ? victim.GetHealth() : -info.amount;
  info.exp = (expDmg / defStat.GetMaxHealth()) * 0.5 * victim.GetProperties()->exp + kill ? victim.GetProperties()->exp : 0;

  return info;
}

/** Calculate the outcome of a magic attack.
 * @param attackerID Entity ID of attacker.
 * @param victim Entity that defends.
 * @param damage Damage to inflict. (Will be scaled by def.)
 * @param element Magic element.
 * @return Result of the attack.
 * @todo Also consider resistance against element.
 */
HealthInfo
Stats::MagicAttack(ID attackerID, const Entity &victim, float damage, Element element) {
  HealthInfo info;
  info.dealerId = attackerID;
  info.type = HealthType::Magic;
  info.element = element;

  // get stats
  Stats defStat = victim.GetEffectiveStats();

  if (damage > 0.0) {
    info.amount = -(damage - defStat.GetMagicDefense());
    if (info.amount > 0.0) info.amount = 0.0;

    bool kill = -info.amount > victim.GetHealth();
    float expDmg = kill ? victim.GetHealth() : -info.amount;
    info.exp = (expDmg / defStat.GetMaxHealth()) * 0.5 * victim.GetProperties()->exp + kill ? victim.GetProperties()->exp : 0;
  } else {
    info.amount = -damage;
    info.exp = 0.0;
  }
  return info;
}

/** Calculate the outcome of an explosion attack.
 * @param attackerID Entity ID that attacks.
 * @param victim Entity that defends.
 * @param damage Damage to inflict.
 * @param element Element of the explosion.
 * @return Result of the attack.
 * @todo Also consider resistance against element.
 */
HealthInfo
Stats::ExplosionAttack(ID attackerID, const Entity &victim, float damage, Element element) {
  HealthInfo info;
  info.dealerId = attackerID;
  info.type = HealthType::Explosion;
  info.element = element;

  // get stats
  Stats defStat = victim.GetEffectiveStats();

  if (damage > 0.0) {
    info.amount = -(damage - defStat.GetDefense());
    if (info.amount > 0.0) info.amount = 0.0;

    bool kill = -info.amount > victim.GetHealth();
    float expDmg = kill ? victim.GetHealth() : -info.amount;
    info.exp = (expDmg / defStat.GetMaxHealth()) * 0.5 * victim.GetProperties()->exp + kill ? victim.GetProperties()->exp : 0;
  } else {
    info.amount = -damage;
    info.exp = 0.0;
  }
  return info;
}

/** Calculate the result of a projectile attack (arrows).
  * @param projectile Projectile that hit the victim.
  * @param victim Victim entity that was hit.
  * @param damage Base damage to do.
  * @return Result of the attack.
  * @todo Also consider resistance against arrows and projectile elements.
  */
HealthInfo
Stats::ProjectileAttack(const Entity &projectile, const Entity &victim, float damage) {
  HealthInfo info;
  info.dealerId = projectile.GetOwner();
  info.type = HealthType::Arrow;
  info.element = projectile.GetElement();

  // get stats
  Stats defStat = victim.GetEffectiveStats();

  if (damage > 0.0) {
    info.amount = -(damage - defStat.GetDefense());
    if (info.amount > 0.0) info.amount = 0.0;
    bool kill = -info.amount > victim.GetHealth();
    float expDmg = kill ? victim.GetHealth() : -info.amount;
    info.exp = (expDmg / defStat.GetMaxHealth()) * 0.5 * victim.GetProperties()->exp + kill ? victim.GetProperties()->exp : 0;
  } else {
    // just healing
    info.amount = damage;
    info.exp = 0.0;
  }
  return info;
}

/** Add some experience.
 * @return true on level up.
 */
bool
Stats::AddExperience(float exp) {
  size_t lvl = Stats::GetLevelForExp(this->GetExperience());
  this->proto.set_exp(this->proto.exp() + exp);
  if (Stats::GetLevelForExp(this->GetExperience()) != lvl) {
    this->proto.set_sp(this->GetSP()+1);
    this->proto.set_max_hp(this->GetMaxHealth()+1);
    return true;
  }
  return false;
}

/** Get the experience points neccessary for a given level.
 * @param lvl Level
 * @return Experience points
 */
float
Stats::GetExpForLevel(size_t lvl) {
  return std::pow(Const::ExpLevelBase, std::pow(lvl - 1, Const::ExpLevelExponent))-1;
}

/** Get the level for the exp.
 * @param exp Experience points
 * @return Level
 */
size_t
Stats::GetLevelForExp(float exp) {
  static constexpr float a = std::pow(std::log(Const::ExpLevelBase), 1.0f/Const::ExpLevelExponent);
  float b = std::pow(std::log(exp+1), 1.0f/Const::ExpLevelExponent);
  return (b + a) / a;
}

/** Get the experience points neccessary for a given level.
 * @param lvl Level
 * @return Experience points
 */
float
Stats::GetExpForSkillLevel(size_t lvl) {
  return std::pow(Const::ExpLevelSkill, std::sqrt(lvl - 1));
}

/** Get the level for the exp.
 * @param exp Experience points
 * @return Level
 */
size_t
Stats::GetLevelForSkillExp(float exp) {
  static constexpr float ln_5_sq = std::log(Const::ExpLevelSkill) * std::log(Const::ExpLevelSkill);
  float l = std::log(exp+1);
  return (l*l + ln_5_sq) / ln_5_sq;
}

/** Get a summary of the stats as a string.
 * @return The tooltip.
 */
std::string
Stats::GetToolTip(bool absolute) const {
  std::string tooltip;
  char tmp[256];

  if (this->GetStrength()) {
    snprintf(tmp, sizeof(tmp), "str: %+d\n", this->GetStrength());
    tooltip += tmp;
  }

  if (this->GetVitality()) {
    snprintf(tmp, sizeof(tmp), "vit: %+d\n", this->GetVitality());
    tooltip += tmp;
  }

  if (this->GetAgility()) {
    snprintf(tmp, sizeof(tmp), "agi: %+d\n", this->GetAgility());
    tooltip += tmp;
  }

  if (this->GetIntelligence()) {
    snprintf(tmp, sizeof(tmp), "int: %+d\n", this->GetIntelligence());
    tooltip += tmp;
  }

  if (this->GetDexterity()) {
    snprintf(tmp, sizeof(tmp), "dex: %+d\n", this->GetDexterity());
    tooltip += tmp;
  }

  if (this->GetLuck()) {
    snprintf(tmp, sizeof(tmp), "luk: %+d\n", this->GetLuck());
    tooltip += tmp;
  }

  if (absolute) {
    if (this->GetAttack()) {
      snprintf(tmp, sizeof(tmp), "atk: %+d\n", this->GetAttack());
      tooltip += tmp;
    }

    if (this->GetDefense()) {
      snprintf(tmp, sizeof(tmp), "def: %+d\n", this->GetDefense());
      tooltip += tmp;
    }

    if (this->GetMagicAttack()) {
      snprintf(tmp, sizeof(tmp), "matk: %+d\n", this->GetMagicAttack());
      tooltip += tmp;
    }

    if (this->GetMagicDefense()) {
      snprintf(tmp, sizeof(tmp), "mdef: %+d\n", this->GetMagicDefense());
      tooltip += tmp;
    }

    if (this->GetHit()) {
      snprintf(tmp, sizeof(tmp), "hit: %+d\n", this->GetHit());
      tooltip += tmp;
    }

    if (this->GetCrit()) {
      snprintf(tmp, sizeof(tmp), "crit: %+d\n", this->GetCrit());
      tooltip += tmp;
    }

    if (this->GetFlee()) {
      snprintf(tmp, sizeof(tmp), "flee: %+d\n", this->GetFlee());
      tooltip += tmp;
    }

    if (this->GetMaxHealth()) {
      snprintf(tmp, sizeof(tmp), "hp:  %+d\n", this->GetMaxHealth());
      tooltip += tmp;
    }
  } else {
    if (this->GetAttackBonus()) {
      snprintf(tmp, sizeof(tmp), "atk: %+d\n", this->GetAttackBonus());
      tooltip += tmp;
    }

    if (this->GetDefenseBonus()) {
      snprintf(tmp, sizeof(tmp), "def: %+d\n", this->GetDefenseBonus());
      tooltip += tmp;
    }

    if (this->GetMagicAttackBonus()) {
      snprintf(tmp, sizeof(tmp), "matk: %+d\n", this->GetMagicAttackBonus());
      tooltip += tmp;
    }

    if (this->GetMagicDefenseBonus()) {
      snprintf(tmp, sizeof(tmp), "mdef: %+d\n", this->GetMagicDefenseBonus());
      tooltip += tmp;
    }

    if (this->GetHitBonus()) {
      snprintf(tmp, sizeof(tmp), "hit: %+d\n", this->GetHitBonus());
      tooltip += tmp;
    }

    if (this->GetCritBonus()) {
      snprintf(tmp, sizeof(tmp), "crit: %+d\n", this->GetCritBonus());
      tooltip += tmp;
    }

    if (this->GetFleeBonus()) {
      snprintf(tmp, sizeof(tmp), "flee: %+d\n", this->GetFleeBonus());
      tooltip += tmp;
    }

    if (this->GetMaxHealthBonus()) {
      snprintf(tmp, sizeof(tmp), "hp:  %+d\n", this->GetMaxHealthBonus());
      tooltip += tmp;
    }
  }
  return tooltip;
}

uint32_t
Stats::GetAttack()       const {
  return GetLevel()/4 + GetStrength() + GetDexterity() / 5 + GetLuck() / 3 + GetAttackBonus(); 
}

uint32_t
Stats::GetMagicAttack()  const {
  return GetLevel()/4 + GetIntelligence() + GetIntelligence()/2 + GetDexterity() / 5 + GetLuck() / 3 + GetMagicAttackBonus(); 
}

uint32_t
Stats::GetDefense()      const {
  return GetVitality() / 2 + std::max(GetVitality()/3, GetVitality()*GetVitality() / 150) + GetDefenseBonus();
}

uint32_t
Stats::GetMagicDefense() const {
  return GetLevel()/4 + GetIntelligence() + GetVitality() / 5 + GetDexterity() / 5;
}

uint32_t
Stats::GetHit()          const {
  return GetHitBonus() + GetDexterity() + GetLuck()/3 + GetLevel();
}

uint32_t
Stats::GetCrit()         const {
  return GetCritBonus() + GetLuck() / 3 + 1;
}

uint32_t
Stats::GetFlee()         const {
  return GetFleeBonus() + GetSkill("evade") + GetLevel() + GetAgility() + GetLuck()/5;
}

uint32_t
Stats::GetMaxHealth()    const {
  return GetMaxHealthBonus() + GetVitality() + GetLevel();
}

/** Compare two stats. Ignores sp and exp. */
bool Stats::operator==(const Stats &o) {
  return 
    this->GetStrength()         == o.GetStrength()      && 
    this->GetDexterity()        == o.GetDexterity()     && 
    this->GetAgility()          == o.GetAgility()       && 
    this->GetDefense()          == o.GetDefense()       &&
    this->GetMagicAttack()      == o.GetMagicAttack()   && 
    this->GetMagicDefense()     == o.GetMagicDefense()  &&
    this->GetWalkSpeed()        == o.GetWalkSpeed()     &&
    this->GetCoolDown()         == o.GetCoolDown()      &&
    this->GetMaxHealth()        == o.GetMaxHealth();
}

uint32_t 
Stats::GetSkill(const std::string &name) const {
  for (auto &s:this->proto.skills()) {
    if (s.name() == name) {
      return Stats::GetLevelForSkillExp(s.exp());
    }
  }
  return 0;
}

bool
Stats::UpgradeSkill(const std::string &name, uint32_t points) {
  uint32_t oldLevel = this->GetSkill(name);

  for (auto &s:*this->proto.mutable_skills()) {
    if (s.name() == name) {
      s.set_exp(s.exp()+points);
      return this->GetSkill(name) > oldLevel;
    }
  }

  Skill_Proto *s = this->proto.add_skills();
  s->set_name(name);
  s->set_exp(points);
  return this->GetSkill(name) > oldLevel;
}

std::unordered_map<std::string, uint32_t> 
Stats::GetAllSkills() {
  std::unordered_map<std::string, uint32_t> skills;

  for (auto &s:this->proto.skills()) {
    skills[s.name()] = Stats::GetLevelForSkillExp(s.exp());
  }
  return skills;
}


/*
Serializer &operator << (Serializer &ser, const Stats &stats) {
  ser << stats.str;
  ser << stats.dex << stats.agi << stats.def;
  ser << stats.maxHealth << stats.exp << stats.sp << stats.walkSpeed;
  ser << stats.skills;
  return ser;
}

Deserializer &operator >> (Deserializer &deser, Stats &stats) {
  deser >> stats.str;
  deser >> stats.dex >> stats.agi >> stats.def;
  deser >> stats.maxHealth >> stats.exp >> stats.sp >> stats.walkSpeed;
  deser >> stats.skills;
  return deser;
}
*/