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

void Particle::Update() {
  Mob::Update();
  
  if (this->dieT == 0.0) this->dieT = Game::Instance->GetTime() + 2 + Game::Instance->GetWorld()->GetRandom().Float();
  if (Game::Instance->GetTime() > this->dieT) this->removable = true;
}

void Particle::Draw(Gfx &gfx) const {
  Entity::Draw(gfx);
}
