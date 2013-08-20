#ifndef BARFOOS_STATS_H
#define BARFOOS_STATS_H

#include "common.h"
#include "icolor.h"
#include "inventory.h"

struct EffectProperties;

namespace Const {
  static constexpr float WalkSpeedFactorPerAGI   = 0.2f; // double walk speed every 5 agi points
  static constexpr float AttackSpeedFactorPerAGI = 0.2f; // double speed every 5 agi points
  static constexpr float ExpLevelBase            = 1.7f; // 1.7^(lvl-1) exp
  static constexpr float ExpLevelSkill           = 1.7f; // 1.1^(lvl-1) exp
};

enum class HealthType : size_t {
  Unspecified = 0,
  Heal,
  Falling,
  Explosion,
  Melee,
  Arrow,
  Vampiric,

  Fire,
  Lava
};

namespace std { template<> struct hash<HealthType> {
  size_t operator()(const HealthType &type) const { return (size_t)type; }
}; }

static inline bool IsContinuous(HealthType t) { return t == HealthType::Fire || t == HealthType::Lava; }

enum class HitType : size_t {
  Miss = 0,
  Normal = 1,
  Critical = 2
};

enum class Element : size_t {
  Physical = 0,
  Fire,
  Water,
  Earth,
  Air,
  Life
};

struct HealthInfo {
  float       amount;
  HealthType  type;
  Element     element;
  ID          dealerId;
  HitType     hitType;
  float       exp;
  std::string skill;

  HealthInfo() :
    amount    (0),
    type      (HealthType::Unspecified),
    element   (Element::Physical),
    dealerId  (InvalidID),
    hitType   (HitType::Normal),
    exp       (0.0),
    skill     ("")
  {}

  HealthInfo(float amount, HealthType type = HealthType::Unspecified, Element element = Element::Physical, size_t dealerId = InvalidID, HitType hitType = HitType::Normal, float exp = 0.0, const std::string &skill = "") :
    amount    (amount),
    type      (type),
    element   (element),
    dealerId  (dealerId),
    hitType   (hitType),
    exp       (exp),
    skill     (skill)
  {}
};

/** A buff/debuff. */
struct Buff {
  /** The effect this buff/debuff has. */
  const EffectProperties *effect;

  /** Time when the buff/debuff was added. */
  float startT;

  friend Serializer &operator << (Serializer &ser, const Buff &buff);
  friend Deserializer &operator >> (Deserializer &deser, Buff &buff);
};

/** Entity stats. */
struct Stats {
  /** Strength, damage done is str * 0.5 * item base damage - def * 0.5. */
  int str = 0;

  /** Dexterity, hit chance is dex:agi. */
  int dex = 0;

  /** Agility. */
  int agi = 0;

  /** Defense. */
  int def = 0;

  /** Maximum number of hitpoints. */
  int maxHealth = 10;

  /** Accumulated experience points. */
  float exp = 0;

  /** Points available to spend on stats. */
  uint32_t sp = 0;

  /** Current walkspeed factor. */
  float walkSpeed = 1.0;

  /** Current cooldown factor. */
  float cooldown = 1.0;

  std::unordered_map<std::string, uint32_t> skills;

  bool operator==(const Stats &o);
  bool AddExp(float exp);
  std::string GetToolTip() const;

  static HealthInfo MeleeAttack(const Entity &attacker, const Entity &victim, const Item &item, Random &random);
  static HealthInfo ExplosionAttack(const Entity &attacker, const Entity &victim, float damage, Element element);

  static float GetExpForLevel(size_t lvl);
  static size_t GetLevelForExp(float exp);
  static float GetExpForSkillLevel(size_t lvl);
  static size_t GetLevelForSkillExp(float exp);

  friend Serializer &operator << (Serializer &ser, const Stats &stats);
  friend Deserializer &operator >> (Deserializer &deser, Stats &stats);
};

#endif
