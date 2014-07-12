#include "game/entities/projectile.h"
#include "game/gamestates/running/runningstate.h"

Projectile::Projectile(const std::string &type) :
  Mob(type)
{
  this->proto.set_spawn_class(uint32_t(SpawnClass::ProjectileClass));
}

void Projectile::Start(RunningState &state, uint32_t id) {
  //Log("Projectile::Start()\n");
  Mob::Start(state, id);

  Vector3 fwd   = GetForward();
  this->AddVelocity(fwd * this->properties->maxSpeed);
}

void
Projectile::OnCollide(RunningState &state, Cell &, Side) {
  Mob::Die(state, HealthInfo());
}

void
Projectile::OnCollide(RunningState &state, Entity &other) {
  float impactDamage = this->GetVelocity().GetSquareMag() * this->properties->impactDamage;
  if (impactDamage != 0.0) {
    other.AddHealth(state, Stats::ProjectileAttack(*this, other, impactDamage));
  }
  Mob::Die(state, HealthInfo());
}
