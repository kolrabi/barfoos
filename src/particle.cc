#include "particle.h"
#include "world.h"
#include "random.h"
#include "game.h"

Particle::Particle(const std::string &type) : 
  Mob(type),
  dieT(0.0) 
{}

Particle::~Particle() {
}

void Particle::Start(Game &game, size_t id) {
  Mob::Start(game, id);
  
  this->dieT = game.GetTime() + 2 + game.GetRandom().Float();
}

void Particle::Update(Game &game) {
  Mob::Update(game);
  
  if (game.GetTime() > this->dieT) this->removable = true;
}
