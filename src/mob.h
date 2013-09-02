#ifndef BARFOOS_MOB_H
#define BARFOOS_MOB_H

#include "common.h"

#include "entity.h"

class Mob : public Entity {
public:

  Mob(const std::string &type);
  Mob(const std::string &type, Deserializer &deser);
  Mob(const Mob &that) = delete;
  virtual ~Mob();
  Mob &                     operator=   (const Mob &that) = delete;

  virtual void              Start       (RunningState &state, ID id)                  override;
  virtual void              Continue    (RunningState &state, ID id)                  override;
  virtual void              Update      (RunningState &state)                         override;
  virtual void              Think       (RunningState &state)                         override;

  virtual void              OnCollide   (RunningState &state, Entity &other)          override;

  void                      ApplyForce  (RunningState &state, const Vector3 &f);
  void                      AddImpulse  (const Vector3 &v)                                            { velocity = velocity + v / this->properties->mass; }
  void                      AddVelocity (const Vector3 &v)                                            { velocity = velocity + v; }
  void                      SetVelocity (const Vector3 &v)                                            { velocity = v; }
  const Vector3 &           GetVelocity ()                                            const           { return velocity; }

  virtual void              Serialize   (Serializer &ser)                             const override;
  virtual SpawnClass        GetSpawnClass()                                           const override { return SpawnClass::MobClass; }

  Cell *                    GetSelection(RunningState &state, float range, const std::shared_ptr<Item> &item, Side &selectedCellSide, ID &entityId);
  bool                      HasLearntSpell(const std::string &name) const;
  virtual void              LearnSpell(const std::string &name);

protected:

  Vector3               lastKnownGoodPos;
  Vector3               velocity;
  Vector3               move;
  bool                  doesWantJump;
  float                 lastJumpT;
  bool                  isOnGround;
  bool                  isInLiquid;
  bool                  isSubmerged;

  bool                  isNoclip;
  bool                  isSneaking;

  std::vector<std::string> learntSpells;

  Cell *                headCell;
  Cell *                footCell;
  Cell *                groundCell;

  void SetInLiquid(bool inLiquid);
  float GetMoveModifier() const;
};



#endif

