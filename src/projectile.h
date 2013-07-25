#ifndef BARFOOS_PROJECTILE_H
#define BARFOOS_PROJECTILE_H

#include "common.h"

#include "mob.h"

class Projectile : public Mob {
public:

  Projectile(const std::string &type);
  Projectile(const std::string &type, Deserializer &deser);

  virtual void Start(RunningState &state, size_t id) override;

  virtual void OnCollide(RunningState &state, Cell &cell, Side side) override;
  
protected:

  virtual SpawnClass GetSpawnClass() const override { return SpawnClass::ProjectileClass; }
};

#endif

