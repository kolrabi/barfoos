#include "particle.h"
#include "world.h"
#include "random.h"
#include "runningstate.h"

Particle::Particle(const std::string &type) : 
  Mob(type),
  dieT(0.0) 
{}

Particle::~Particle() {
}

void Particle::Start(RunningState &state, size_t id) {
  Mob::Start(state, id);
  
  this->dieT = state.GetGame().GetTime() + 2 + state.GetRandom().Float();
}

void Particle::Update(RunningState &state) {
  Mob::Update(state);
  
  if (state.GetGame().GetTime() > this->dieT) {
    this->Die(state, HealthInfo());
    this->removable = true;
  }
}
