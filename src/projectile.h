#ifndef BARFOOS_PROJECTILE_H
#define BARFOOS_PROJECTILE_H

#include "common.h"

#include "mob.h"

class Projectile : public Mob {
public:

  Projectile(const std::string &type);
  virtual ~Projectile();

  virtual void Start(Game &game, size_t id) override;
  virtual void Update(Game &game) override;

  virtual void OnCollide(Game &game, Cell &cell, Side side) override;
  
protected:

  float dieT;
};

#endif

