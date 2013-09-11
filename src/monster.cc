#include "monster.h"
#include "world.h"
#include "cell.h"
#include "util.h"
#include "runningstate.h"
#include "item.h"

#include "audio.h"

#include "serializer.h"
#include "deserializer.h"

#include <cmath>

Monster::Monster(const std::string &propertyName) :
  Mob             (propertyName)
{
  this->proto.set_spawn_class(uint32_t(SpawnClass::MonsterClass));
  this->proto.mutable_monster();
}

Monster::Monster(const Entity_Proto &proto) :
  Mob             (proto)
{
}

Monster::~Monster() {
}

void
Monster::Start(RunningState &state, ID id) {
  Mob::Start(state, id);

  if (properties->attackItem != "") {
    this->attackItem = std::shared_ptr<Item>(new Item(properties->attackItem));
    this->attackItem->Update(state);
  }
}

void
Monster::Continue(RunningState &state, ID id) {
  Mob::Continue(state, id);

  if (properties->attackItem != "") {
    this->attackItem = std::shared_ptr<Item>(new Item(properties->attackItem));
    this->attackItem->Update(state);
  }
}

void
Monster::Update(RunningState &state) {
  Mob::Update(state);

  if (this->IsDead()) return;
  if (this->attackItem) this->attackItem->Update(state);
}

void
Monster::Think(RunningState &state) {
  Mob::Think(state);

  if (this->IsDead()) {
    this->ClearMoveTarget();
    return;
  }

  //Log("Monster::Think: %u %s\n", this->GetId(), this->GetName().c_str());

  if (this->IsAttackTargetValid()) {
    Entity *enemy = state.GetEntity(this->GetAttackTarget());

    if (enemy) {
      //Log("  It is valid: %p (%s)\n", enemy, enemy->GetName().c_str());

      float dist = (enemy->GetPosition() - GetPosition()).GetMag();

      if (dist > this->properties->aggroRangeFar) {
        // out of range? -> unset attack target
        this->ClearAttackTarget();
      } else if (!CanSee(state, enemy->GetPosition())) {
        // not visible? -> set move target to last known location, unset attack target
        this->SetMoveTarget(enemy->GetPosition());
        this->ClearAttackTarget();
      } else if (dist < this->properties->meleeAttackRange &&
                 dist >= this->properties->keepDistance &&
                 state.GetGame().GetTime() > this->GetNextAttackTime()) {
        //Log("  I chose to attack it...\n");
        this->sprite.StartAnim(this->properties->attackAnim);

        if (this->properties->attackForwardStep) {
          this->AddVelocity((enemy->GetPosition() - GetPosition()).Horiz().Normalize() * this->properties->attackForwardStep);
        }

        if (this->attackItem) {
          this->attackItem->UseOnEntity(state, *this, this->GetAttackTarget());
        }

        this->AddVelocity(Vector3(0, this->properties->attackJump, 0));
        this->SetNextAttackTime(state.GetGame().GetTime() + this->properties->attackInterval);
        this->PlaySound(state, "attack");
      }

    } else {
      this->ClearAttackTarget();
    }
  }

  if (!this->IsAttackTargetValid() && this->properties->aggressive) {
    // Log("  I don't have a target, but I am aggressive and looking for one...\n");
    // find enitites in aggroRange
    std::vector<ID> ents = state.FindEntities(aabb.center, this->properties->aggroRangeNear);
    Entity *enemy = nullptr;

    for (size_t e : ents) {
      Entity *entity = state.GetEntity(e);

      if (!entity) continue;

      // suitable? -> set attack target
      if (entity->GetProperties()->name == "player" && CanSee(state, entity->GetPosition())) {
        enemy = entity;
        // Log("  Found one: %u %s\n", enemy->GetId(), enemy->GetName().c_str());
        this->PlaySound(state, "engage");
        break;
      }
    }

    if (enemy) {
      this->SetAttackTarget(enemy->GetId());
    }
  }

  if (this->IsAttackTargetValid()) {
    // walk toward enemy
    Entity *enemy = state.GetEntity(this->GetAttackTarget());

    if (enemy) {
      Vector3 dir    = enemy->GetPosition() - GetPosition();

      // look towards enemy
      Vector3 fwd    = dir.Normalize();
      this->SetForward(fwd);

      // dont get too close
      dir = dir - fwd * this->properties->keepDistance;

      Vector3 dhoriz = dir.Horiz();
      move = dhoriz.Normalize() * this->properties->maxSpeed;

    } else {
      this->ClearAttackTarget();
    }

  } else if (!this->IsAttackTargetValid() && this->properties->moveInterval != 0) {
    // walk around a bit
    if (state.GetGame().GetTime() > this->GetNextMoveTime()) {
      this->SetNextMoveTime(this->GetNextMoveTime() + this->properties->moveInterval);
      this->SetMoveTarget(aabb.center + (Vector3(state.GetRandom().Float(), state.GetRandom().Float(), state.GetRandom().Float())) * 4.0);
    }

    if (this->IsMoveTargetValid()) {
      Vector3 tmove = (this->GetMoveTarget() - aabb.center).Horiz();

      if (tmove.GetMag() < 0.1) {
        this->ClearMoveTarget();
      } else {
        this->SetForward((this->GetMoveTarget()-aabb.center).Normalize());
        this->move = tmove.Normalize() * this->properties->maxSpeed;
      }
    }
  }
}

void
Monster::OnCollide(RunningState &state, Entity &other) {
  Mob::OnCollide(state, other);

  if (this->IsSolid()) return;

  if (this->properties->attackForwardStep && this->IsAttackTargetValid() && other.GetId() == this->GetAttackTarget()) {
    this->AddVelocity(-(other.GetPosition() - GetPosition()).Horiz().Normalize() * this->properties->attackForwardStep);

    // TODO: if can use on entity
    if (this->attackItem) this->attackItem->UseOnEntity(state, *this, other.GetId());

    return;
  }

  Vector3 d = this->GetAABB().center - other.GetAABB().center;
  Vector3 f = d * (this->properties->mass * other.GetProperties()->mass / (1 + d.GetSquareMag()));
  this->ApplyForce(state, f);
}

void
Monster::AddHealth(RunningState &state, const HealthInfo &info) {
  Mob::AddHealth(state, info);
  if (info.amount < 0 &&
      info.dealerId != InvalidID &&
      info.dealerId != this->GetId() &&
      (this->properties->aggressive || this->properties->retaliate)) {
    this->SetAttackTarget(info.dealerId);
  }
}
