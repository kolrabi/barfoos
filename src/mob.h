#ifndef BARFOOS_MOB_H
#define BARFOOS_MOB_H

#include "common.h"

#include "entity.h"

class Mob : public Entity {
public:

  Mob(const std::string &entityPropertyName);
  Mob(const Mob &that) = delete;
  virtual ~Mob();
  
  Mob &operator=(const Mob &that) = delete;

  virtual void Start(RunningState &state, size_t id) override;
  virtual void Update(RunningState &state) override;
  virtual void Think(RunningState &state) override;
  
  virtual void Die(RunningState &state, const HealthInfo &info) override;
  virtual void OnCollide(RunningState &state, Entity &other) override;
  
  void ApplyForce(RunningState &state, const Vector3 &f);
  void AddImpulse(const Vector3 &v)  { velocity = velocity + v / this->properties->mass; }
  void AddVelocity(const Vector3 &v) { velocity = velocity + v; }
  const Vector3 &GetVelocity() const { return velocity; }

protected:

  Vector3 velocity, move;
  bool wantJump;
  float lastJumpT;
  bool onGround;
  bool inWater;
  bool underWater;
  
  bool noclip;
  bool sneak;
  
  float nextMoveT;
  Vector3 moveTarget;
  bool validMoveTarget;

  Cell *headCell, *footCell, *groundCell;

  void SetInLiquid(bool inLiquid);
  float GetMoveModifier() const;

  virtual SpawnClass GetSpawnClass() const override { return SpawnClass::MobClass; }
};



#endif

