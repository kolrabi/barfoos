#ifndef BARFOOS_PARTICLE_H
#define BARFOOS_PARTICLE_H

#include "common.h"

#include "mob.h"

class Item;

class Particle : public Mob {
public:

  Particle(const std::string &type);
  virtual ~Particle();

  virtual void Start(Game &game, size_t id) override;
  virtual void Update(Game &game) override;
  
protected:

  float dieT;
};

#endif

