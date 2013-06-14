#ifndef BARFOOS_MOB_H
#define BARFOOS_MOB_H


#include "common.h"

#include "entity.h"

class World;
class Cell;

class Mob : public Entity {
public:

  Mob();
  virtual ~Mob();

  void SetSpawnPos(const Vector3 &p) { this->spawnPos = p; }
  const Vector3 &GetAngles() const { return angles; }

  virtual void Update(float t);
  
  void ApplyForce(const Vector3 &f) { velocity = velocity + f * (deltaT/mass); }
  virtual void Die();

protected:

  Vector3 spawnPos;
  Vector3 angles;
  Vector3 velocity, move;
  bool wantJump;
  float lastJumpT;
  bool onGround;
  bool inWater;
  bool underWater;
  float stepHeight;
  float mass;

  bool noclip;
  bool sneak;
  
  float moveInterval;
  float nextMoveT;
  Vector3 moveTarget;
  bool validMoveTarget;
  
  float maxSpeed;

  Cell *headCell, *footCell, *groundCell;

  void SetInLiquid(bool inLiquid);
};



#endif

