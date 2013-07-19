#ifndef BARFOOS_STATS_H
#define BARFOOS_STATS_H

#include "common.h"
#include "icolor.h"
#include "inventory.h"

namespace Const {
  static constexpr float WalkSpeedFactorPerAGI   = 0.2f; // double walk speed every 5 agi points
  static constexpr float AttackSpeedFactorPerAGI = 0.2f; // double speed every 5 agi points
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
  Wind
};

struct HealthInfo {
  float amount = 0;
  HealthType type = HealthType::Unspecified;
  Element element = Element::Physical;
  size_t dealerId = ~0UL;
  HitType hitType = HitType::Normal;
  float exp = 0.0;
  std::string skill = "";
  
  HealthInfo() {}
  HealthInfo(float amount, HealthType type = HealthType::Unspecified, Element element = Element::Physical, size_t dealerId = ~0UL, HitType hitType = HitType::Normal, float exp = 0.0, const std::string &skill = "") : 
    amount(amount), 
    type(type), 
    element(element), 
    dealerId(dealerId),
    hitType(hitType),
    exp(exp),
    skill(skill)
  {}
};

struct Stats {
  int str = 0;
  int dex = 0;
  int agi = 0;
  int def = 0;
  
  int maxHealth = 10;
  
  float exp = 0;
  
  size_t sp = 0;
  
  bool AddExp(float exp);
  size_t GetLevel();
  
  std::string GetToolTip() const;
  
  static HealthInfo MeleeAttack(const Entity &attacker, const Entity &victim, const Item &item, Random &random);
  static HealthInfo ExplosionAttack(const Entity &attacker, const Entity &victim, float damage, Element element);
  static float GetExpForLevel(size_t lvl);
};

#endif
