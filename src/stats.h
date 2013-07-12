#ifndef BARFOOS_STATS_H
#define BARFOOS_STATS_H

#include "common.h"
#include "icolor.h"
#include "inventory.h"

#include "stats.h"

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
  
  HealthInfo() {}
  HealthInfo(float amount, HealthType type = HealthType::Unspecified, Element element = Element::Physical, size_t dealerId = ~0UL) 
    : amount(amount), type(type), element(element), dealerId(dealerId) { }
};

struct Stats {
  int str = 1;
  int dex = 1;
  int wis = 1;
  int agi = 1;
};

#endif
