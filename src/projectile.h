#ifndef BARFOOS_PROJECTILE_H
#define BARFOOS_PROJECTILE_H

#include "common.h"

#include "mob.h"

class Projectile : public Mob {
public:

  Projectile(const std::string &type);
  virtual ~Projectile();

  virtual void Start(RunningState &state, size_t id) override;
  virtual void Update(RunningState &state) override;

  virtual void OnCollide(RunningState &state, Cell &cell, Side side) override;
  
protected:

  float dieT;
};

#endif

