#include "projectile.h"
#include "runningstate.h"

Projectile::Projectile(const std::string &type) :
  Mob(type)
{}

Projectile::Projectile(const std::string &type, Deserializer &deser) : Mob(type, deser) {
}

void Projectile::Start(RunningState &state, uint32_t id) {
  Log("Projectile::Start()\n");
  Mob::Start(state, id);
  
  Vector3 fwd   = GetForward();
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
