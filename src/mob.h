#ifndef BARFOOS_MOB_H
#define BARFOOS_MOB_H

#include "common.h"

#include "entity.h"

class World;
class Cell;

class Mob : public Entity {
public:

  Mob(const std::string &entityPropertyName);
  virtual ~Mob();

  void SetSpawnPos(const Vector3 &p) { this->spawnPos = p; }
  const Vector3 &GetAngles() const { return angles; }
  void SetAngles(const Vector3 &angles) { this->angles = angles; }

  virtual void Start(Game &game, size_t id) override;
  virtual void Update(Game &game) override;
  virtual void Think(Game &game) override;
  
  virtual void Die(Game &game, const HealthInfo &info) override;
  virtual void OnCollide(Game &game, Entity &other) override;
  
  void ApplyForce(Game &game, const Vector3 &f);
  void AddVelocity(const Vector3 &v) { velocity = velocity + v; }

protected:

  Vector3 spawnPos;
  Vector3 angles;
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
};



#endif

