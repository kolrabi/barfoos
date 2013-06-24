#include "particle.h"
#include "world.h"
#include "random.h"

Particle::Particle() : 
  Mob("particle") {
  this->dieT = 0;
}

Particle::~Particle() {
}

void Particle::Update(float t) {
  if (this->dieT == 0.0) this->dieT = t+2+ this->world->GetRandom().Float();
  
  Mob::Update(t);
  if (t > this->dieT) this->removable = true;
}

void Particle::Draw() {
  Entity::Draw();
}
