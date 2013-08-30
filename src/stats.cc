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

  float charge = item.NeedsChargeUp() ? item.GetCharge() : 0.0;

  // get stats
  Stats atkStat = attacker.GetEffectiveStats();
  Stats defStat = victim.GetEffectiveStats();

  uint32_t atkSkillLevel = Stats::GetLevelForSkillExp(atkStat.skills[info.skill]);
  uint32_t defSkillLevel = Stats::GetLevelForSkillExp(defStat.skills["evade"]);

  // determine hit
  int dex = random.Integer(std::max(atkStat.dex + atkSkillLevel, (uint32_t)1));
  int agi = random.Integer(std::max(defStat.agi + defSkillLevel, (uint32_t)1));

  HitType hit = (dex < agi) ? HitType::Miss : HitType::Normal;
  if (random.Chance(atkStat.dex*0.001)) hit = HitType::Critical;

  info.hitType = hit;

  int dmg = (atkStat.str * 0.5 * item.GetDamage() + atkSkillLevel*0.5) * charge;
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

  /** Calculate the outcome of an explosion attack.
   * @param attacker Entity that attacks
   * @param victim Entity that defends
   * @param damage Damage to inflict
   * @param random A random number generator for variance
   * @return Result of the attack.
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
    info.amount = -(damage - defStat.def * 0.5);
    if (info.amount > 0.0) info.amount = 0.0;

    bool kill = -info.amount > victim.GetHealth();
    float expDmg = kill ? victim.GetHealth() : -info.amount;
    info.exp = (expDmg / defStat.maxHealth) * 0.5 * victim.GetProperties()->exp + kill ? victim.GetProperties()->exp : 0;
  } else {
    info.amount = -damage;
    info.exp = 0.0;
  }
  return info;
}

HealthInfo
Stats::ProjectileAttack(const Entity &projectile, const Entity &victim, float damage) {
  HealthInfo info;
  info.dealerId = projectile.GetOwner();
  info.type = HealthType::Arrow;
  info.element = projectile.GetElement();

  // get stats
  Stats defStat = victim.GetEffectiveStats();

  if (damage > 0.0) {
    info.amount = -(damage - defStat.def * 0.5);
    if (info.amount > 0.0) info.amount = 0.0;
    bool kill = -info.amount > victim.GetHealth();
    float expDmg = kill ? victim.GetHealth() : -info.amount;
    info.exp = (expDmg / defStat.maxHealth) * 0.5 * victim.GetProperties()->exp + kill ? victim.GetProperties()->exp : 0;
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
Stats::AddExp(float exp) {
  size_t lvl = Stats::GetLevelForExp(this->exp);
  this->exp += exp;
  if (Stats::GetLevelForExp(this->exp) != lvl) {
    this->sp ++;
    this->maxHealth ++;
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
  return std::pow(Const::ExpLevelBase, lvl - 1);
}

/** Get the level for the exp.
 * @param exp Experience points
 * @return Level
 */
size_t
Stats::GetLevelForExp(float exp) {
  return std::log(exp + 1) / std::log(Const::ExpLevelBase) + 1;
}

/** Get the experience points neccessary for a given level.
 * @param lvl Level
 * @return Experience points
 */
float
Stats::GetExpForSkillLevel(size_t lvl) {
  return std::pow(Const::ExpLevelSkill, lvl - 1);
}

/** Get the level for the exp.
 * @param exp Experience points
 * @return Level
 */
size_t
Stats::GetLevelForSkillExp(float exp) {
  return std::log(exp + 1) / std::log(Const::ExpLevelSkill) + 1;
}

/** Get a summary of the stats as a string.
 * @return The tooltip.
 */
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

/** Compare two stats. Ignores sp and exp. */
bool Stats::operator==(const Stats &o) {
  return str == o.str && dex == o.dex && agi == o.agi && def == o.def &&
         walkSpeed == o.walkSpeed && maxHealth == o.maxHealth;
}

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

// ==========================================================================

Serializer &operator << (Serializer &ser, const Buff &buff) {
  ser << buff.effect->name << buff.startT;
  return ser;
}

Deserializer &operator >> (Deserializer &deser, Buff &buff) {
  std::string effectName;
  deser >> effectName;
  buff.effect = &getEffect(effectName);

  deser >> buff.startT;
  return deser;
}
