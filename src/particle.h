#ifndef BARFOOS_PARTICLE_H
#define BARFOOS_PARTICLE_H

#include "common.h"

#include "mob.h"

class Item;

class Particle : public Mob {
public:

  Particle(const std::string &type);
  virtual ~Particle();

  virtual void Start(RunningState &state, size_t id) override;
  virtual void Update(RunningState &state) override;
  
protected:

  float dieT;
};

#endif

