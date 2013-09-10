#include "common.h"

#include "stats.h"

#include "item.h"
#include "entity.h"
#include "random.h"

#include "serializer.h"
#include "deserializer.h"

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

  uint32_t atkSkillLevel = atkStat.GetSkill(info.skill);
  uint32_t defSkillLevel = defStat.GetSkill("evade");

  // determine hit
  int dex = random.Integer(std::max(atkStat.GetDexterity() + atkSkillLevel, (uint32_t)1));
  int agi = random.Integer(std::max(defStat.GetAgility()   + defSkillLevel, (uint32_t)1));

  HitType hit = (dex < agi) ? HitType::Miss : HitType::Normal;
  if (random.Chance(atkStat.GetDexterity()*0.001)) hit = HitType::Critical;

  info.hitType = hit;

  int dmg = (atkStat.GetStrength() * 0.5 * item.GetDamage() + atkSkillLevel*0.5) * charge;
  if (dmg < 1) dmg = 1;

  int amount = dmg - defStat.GetDefense() * 0.1;
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
    info.amount = -(damage - defStat.GetMagicDefense() * 0.5);
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
    info.amount = -(damage - defStat.GetDefense() * 0.5);
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
    info.amount = -(damage - defStat.GetDefense() * 0.5);
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
Stats::GetToolTip() const {
  std::string tooltip;
  char tmp[256];

  if (this->GetStrength()) {
    snprintf(tmp, sizeof(tmp), "str: %+d\n", this->GetStrength());
    tooltip += tmp;
  }

  if (this->GetAgility()) {
    snprintf(tmp, sizeof(tmp), "agi: %+d\n", this->GetAgility());
    tooltip += tmp;
  }

  if (this->GetDexterity()) {
    snprintf(tmp, sizeof(tmp), "dex: %+d\n", this->GetDexterity());
    tooltip += tmp;
  }

  if (this->GetDefense()) {
    snprintf(tmp, sizeof(tmp), "def: %+d\n", this->GetDefense());
    tooltip += tmp;
  }

  if (this->GetMaxHealth()) {
    snprintf(tmp, sizeof(tmp), "hp:  %+d\n", this->GetMaxHealth());
    tooltip += tmp;
  }
  return tooltip;
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