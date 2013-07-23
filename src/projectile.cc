#include "projectile.h"
#include "world.h"
#include "random.h"
#include "runningstate.h"
#include "worldedit.h"

Projectile::Projectile(const std::string &type) :
  Mob(type),
  dieT(0.0)
{}

Projectile::~Projectile() {
}

void Projectile::Start(RunningState &state, size_t id) {
  Mob::Start(state, id);
  
  Vector3 fwd   = (GetAngles()).EulerToVector();
  this->SetPosition(state.GetWorld().MoveAABB(this->aabb, this->GetPosition() + fwd));
  this->AddVelocity(fwd * this->properties->maxSpeed);

  this->dieT = state.GetGame().GetTime() + 10 + state.GetRandom().Float();
}

void Projectile::Update(RunningState &state) {
  Mob::Update(state);
  
  if (state.GetGame().GetTime() > this->dieT) {
    this->Die(state, HealthInfo());
  }
}

void 
Projectile::OnCollide(RunningState &state, Cell &, Side) {
  Mob::Die(state, HealthInfo());
  this->removable = true;
  
  // TODO: from properties
  state.Explosion(*this, this->GetPosition(), 3, 1.0, 10, Element::Physical);
}
