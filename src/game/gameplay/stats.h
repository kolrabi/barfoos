#ifndef BARFOOS_STATS_H
#define BARFOOS_STATS_H

#include <vector>
#include <unordered_map>

#include "entity.pb.h"

struct EffectProperties;
const EffectProperties &getEffect(const std::string &name);

namespace Const {
  static constexpr float WalkSpeedFactorPerAGI   = 0.2f; // double walk speed every 5 agi points
  static constexpr float AttackSpeedFactorPerAGI = 0.2f; // double speed every 5 agi points
  static constexpr float ExpLevelBase            = 5.0f; //
  static constexpr float ExpLevelExponent        = 0.4f; //
  static constexpr float ExpLevelSkill           = 3.0f; //
};

enum class HitType : uint8_t {
  Miss = 0,
  Normal = 1,
  Critical = 2
};

enum class Element : uint8_t {
  Physical = 0,
  Fire,
  Water,
  Earth,
  Wind,
  Life
};

namespace std { template<> struct hash<Element> {
  size_t operator()(const Element &type) const { return (size_t)type; }
}; }

enum class HealthType : uint8_t {
  Unspecified = 0,
  Heal,
  Falling,
  Explosion,
  Melee,
  Arrow,
  Vampiric,

  Magic,

  Fire,
  Lava
};

namespace std { template<> struct hash<HealthType> {
  size_t operator()(const HealthType &type) const { return (size_t)type; }
}; }

static inline bool IsContinuous(HealthType t) { return t == HealthType::Fire || t == HealthType::Lava; }

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
  Buff(const std::string &type, float startT) {
    this->proto.set_effect(type);
    this->proto.set_start_time(startT);
  }

  Buff(const Buff_Proto &proto) : proto(proto) {}

  const EffectProperties &GetEffect()     const { return getEffect(this->proto.effect()); }
  float                   GetStartTime()  const { return this->proto.start_time(); }

  void                    Extend(float t)       { this->proto.set_start_time(this->proto.start_time() + t); }
  void                    Restart(float t)      { this->proto.set_start_time(t); }

  Buff_Proto proto;
};

/** Entity stats. */
struct Stats {
  Stats() {
  }
  Stats(const Stats_Proto &proto) : proto(proto) {}

  // primary stats
  uint32_t GetStrength()     const { return proto.str();     }
  uint32_t GetAgility()      const { return proto.agi();     }
  uint32_t GetVitality()     const { return proto.dex();     }
  uint32_t GetIntelligence() const { return proto.int_();    }
  uint32_t GetDexterity()    const { return proto.dex();     }
  uint32_t GetLuck()         const { return proto.luk();     }

  void SetStrength(uint32_t v)      { proto.set_str(v);     }
  void SetAgility(uint32_t v)       { proto.set_agi(v);     }
  void SetVitality(uint32_t v)      { proto.set_vit(v);     }
  void SetIntelligence(uint32_t v)  { proto.set_int_(v);    }
  void SetDexterity(uint32_t v)     { proto.set_dex(v);     }
  void SetLuck(uint32_t v)          { proto.set_luk(v);    }

  // substats 
  uint32_t GetAttack()       const;
  uint32_t GetMagicAttack()  const;
  uint32_t GetDefense()      const;
  uint32_t GetMagicDefense() const;
  uint32_t GetHit()          const;
  uint32_t GetCrit()         const;
  uint32_t GetFlee()         const;

  // boni
  int32_t GetAttackBonus()       const { return proto.atk_bonus(); }
  int32_t GetDefenseBonus()      const { return proto.def_bonus(); }
  int32_t GetMagicAttackBonus()  const { return proto.matk_bonus(); }
  int32_t GetMagicDefenseBonus() const { return proto.mdef_bonus(); }
  int32_t GetHitBonus()          const { return proto.hit_bonus(); }
  int32_t GetCritBonus()         const { return proto.crit_bonus(); }
  int32_t GetFleeBonus()         const { return proto.flee_bonus(); }

  void SetAttackBonus(int32_t v)         { proto.set_atk_bonus(v);    }
  void SetDefenseBonus(int32_t v)        { proto.set_def_bonus(v);     }
  void SetMagicAttackBonus(int32_t v)    { proto.set_matk_bonus(v);    }
  void SetMagicDefenseBonus(int32_t v)   { proto.set_mdef_bonus(v);    }
  void SetHitBonus(int32_t v)            { proto.set_hit_bonus(v);    }
  void SetCritBonus(int32_t v)           { proto.set_crit_bonus(v);    }
  void SetFleeBonus(int32_t v)           { proto.set_flee_bonus(v);    }

  uint32_t GetMaxHealth()    const;

  uint32_t GetMaxHealthBonus()    const { return proto.max_hp(); }
  void SetMaxHealthBonus(uint32_t v)     { proto.set_max_hp(v);  }

  void SetExperience(float exp) { this->proto.set_exp(exp); }
  float GetExperience() const { return proto.exp();     }
  uint32_t GetLevel()   const { return GetLevelForExp(this->GetExperience()); }
  
  void SetSP(uint32_t sp)     { this->proto.set_sp(sp); }
  uint32_t GetSP()      const { return proto.sp();      }

  float GetWalkSpeed()  const { return proto.walk_speed(); }
  float GetCoolDown()   const { return proto.cool_down(); }
  void  SetWalkSpeed(float f) { proto.set_walk_speed(f); }
  void  SetCoolDown(float f)  { proto.set_cool_down(f); }

  uint32_t GetSkill(const std::string &name) const;
  bool UpgradeSkill(const std::string &name, uint32_t points = 1);
  std::unordered_map<std::string, uint32_t> GetAllSkills();

  bool operator==(const Stats &o);
  bool AddExperience(float exp);
  std::string GetToolTip(bool absolute=false) const;

  Stats_Proto proto;

  static HealthInfo MeleeAttack(const Entity &attacker, const Entity &victim, const Item &item, Random &random);
  static HealthInfo MagicAttack(ID attackerID, const Entity &victim, float damage, Element element);
  static HealthInfo ExplosionAttack(ID attackerID, const Entity &victim, float damage, Element element);
  static HealthInfo ProjectileAttack(const Entity &projectile, const Entity &victim, float damage);

  static float GetExpForLevel(size_t lvl);
  static size_t GetLevelForExp(float exp);
  static float GetExpForSkillLevel(size_t lvl);
  static size_t GetLevelForSkillExp(float exp);
};

#endif
