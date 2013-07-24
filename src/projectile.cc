#include "projectile.h"
#include "world.h"
#include "random.h"
#include "runningstate.h"
#include "worldedit.h"

Projectile::Projectile(const std::string &type) :
  Mob(type)
{}

Projectile::~Projectile() {
}

void Projectile::Start(RunningState &state, size_t id) {
  Mob::Start(state, id);
  
  Vector3 fwd   = (GetAngles()).EulerToVector();
  this->SetPosition(state.GetWorld().MoveAABB(this->aabb, this->GetPosition() + fwd));
  this->AddVelocity(fwd * this->properties->maxSpeed);
}

void Projectile::Update(RunningState &state) {
  Mob::Update(state);
}

void 
Projectile::OnCollide(RunningState &state, Cell &, Side) {
  Mob::Die(state, HealthInfo());
  this->removable = true;
  
  // TODO: from properties
  state.Explosion(*this, this->GetPosition(), 3, 1.0, 10, Element::Physical);
}
