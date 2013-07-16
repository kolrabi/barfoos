#ifndef BARFOOS_PARTICLE_H
#define BARFOOS_PARTICLE_H

#include "common.h"

#include "mob.h"

class Item;

class Particle : public Mob {
public:

  Particle();
  virtual ~Particle();

  virtual void Update(Game &game) override;
  
protected:

  float dieT;
};

#endif

