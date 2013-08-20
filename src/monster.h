#ifndef BARFOOS_MONSTER_H
#define BARFOOS_MONSTER_H

#include "common.h"

#include "mob.h"

class Monster : public Mob {
public:

  Monster(const std::string &type);
  Monster(const std::string &type, Deserializer &deser);
  Monster(const Monster &that) = delete;
  virtual ~Monster();
  Monster &                 operator=   (const Monster &that) = delete;

  virtual void              Start       (RunningState &state, ID id)                  override;
  virtual void              Continue    (RunningState &state, ID id)                  override;
  virtual void              Update      (RunningState &state)                         override;
  virtual void              Think       (RunningState &state)                         override;

  virtual void              OnCollide   (RunningState &state, Entity &other)          override;
  virtual void              AddHealth(RunningState &state, const HealthInfo &info)    override;

  virtual void              Serialize   (Serializer &ser)                             const override;
  virtual SpawnClass        GetSpawnClass()                                           const override { return SpawnClass::MonsterClass; }

protected:

  float                 nextMoveT;
  Vector3               moveTarget;
  bool                  isMoveTargetValid;

  ID                    attackTarget;
  float                 nextAttackT;
  std::shared_ptr<Item> attackItem;
};



#endif

