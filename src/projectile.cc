#include "projectile.h"
#include "world.h"
#include "random.h"
#include "runningstate.h"
#include "worldedit.h"

Projectile::Projectile(const std::string &type) :
  Mob(type)
{}

Projectile::Projectile(const std::string &type, Deserializer &deser) : Mob(type, deser) {
}

void Projectile::Start(RunningState &state, size_t id) {
  Mob::Start(state, id);
  
  Vector3 fwd   = (GetAngles()).EulerToVector();
  this->SetPosition(state.GetWorld().MoveAABB(this->aabb, this->GetPosition() + fwd));
  this->AddVelocity(fwd * this->properties->maxSpeed);
}

void 
Projectile::OnCollide(RunningState &state, Cell &, Side) {
  Mob::Die(state, HealthInfo());
}

void 
Projectile::OnCollide(RunningState &state, Entity &) {
  Mob::Die(state, HealthInfo());
}
