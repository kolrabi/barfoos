#ifndef BARFOOS_PROJECTILE_H
#define BARFOOS_PROJECTILE_H

#include "common.h"

#include "game/entities/mob.h"

class Projectile : public Mob {
public:

  Projectile(const std::string &type);

  virtual void Start(RunningState &state, uint32_t id) override;

  virtual void OnCollide(RunningState &state, Cell &cell, Side side) override;
  virtual void OnCollide(RunningState &state, Entity &) override;
  
protected:
};

#endif

