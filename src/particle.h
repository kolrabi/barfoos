#ifndef BARFOOS_ITEMENTITY_H
#define BARFOOS_ITEMENTITY_H

#include "common.h"

#include "mob.h"

class Item;

class Particle : public Mob {
public:

  Particle();
  virtual ~Particle();

  virtual void Update(float t);
  virtual void Draw();
  
protected:

  float dieT;
};

#endif

