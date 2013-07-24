#include "particle.h"
#include "world.h"
#include "random.h"
#include "runningstate.h"

// has this become redundant?

Particle::Particle(const std::string &type) : 
  Mob(type)
{}

Particle::~Particle() {
}

void Particle::Start(RunningState &state, size_t id) {
  Mob::Start(state, id);
}

void Particle::Update(RunningState &state) {
  Mob::Update(state);
}
