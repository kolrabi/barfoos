#ifndef BARFOOS_STATS_H
#define BARFOOS_STATS_H

#include "common.h"
#include "icolor.h"
#include "inventory.h"

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
  int str = 10;
  int dex = 10;
  int agi = 10;
  int def = 10;
  
  int maxHealth = 10;
  
  float exp = 0;
  
  size_t sp = 0;
  
  bool AddExp(float exp);
  size_t GetLevel();
  
  static HealthInfo MeleeAttack(const Entity &attacker, const Entity &victim, const Item &item, Random &random);
  static float GetExpForLevel(size_t lvl);
};

#endif
