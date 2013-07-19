#include "projectile.h"
#include "world.h"
#include "random.h"
#include "game.h"
#include "worldedit.h"

Projectile::Projectile(const std::string &type) :
  Mob(type),
  dieT(0.0)
{}

Projectile::~Projectile() {
}

void Projectile::Start(Game &game, size_t id) {
  Mob::Start(game, id);
  
  Vector3 fwd   = (GetAngles()).EulerToVector();
  this->SetPosition(game.GetWorld().MoveAABB(this->aabb, this->GetPosition() + fwd));
  this->AddVelocity(fwd * this->properties->maxSpeed);

  this->dieT = game.GetTime() + 10 + game.GetRandom().Float();
}

void Projectile::Update(Game &game) {
  Mob::Update(game);
  
  if (game.GetTime() > this->dieT) {
    this->Die(game, HealthInfo());
  }
}

void 
Projectile::OnCollide(Game &game, Cell &cell, Side side) {
  (void)cell;
  (void)side;
  
  Mob::Die(game, HealthInfo());
  this->removable = true;
  
  game.Explosion(*this, this->GetPosition(), 3, 1.0, 10, Element::Physical);
}
