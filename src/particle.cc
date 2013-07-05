#include "particle.h"
#include "world.h"
#include "random.h"
#include "game.h"

Particle::Particle() : 
  Mob("particle") {
  this->dieT = 0.0;
}

Particle::~Particle() {
}

void Particle::Update(Game &game) {
  Mob::Update(game);
  
  if (this->dieT == 0.0) this->dieT = game.GetTime() + 2 + game.GetRandom().Float();
  if (game.GetTime() > this->dieT) this->removable = true;
}

void Particle::Draw(Gfx &gfx) const {
  Entity::Draw(gfx);
}
