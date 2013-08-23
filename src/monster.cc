#include "monster.h"
#include "world.h"
#include "cell.h"
#include "util.h"
#include "runningstate.h"
#include "item.h"

#include "serializer.h"
#include "deserializer.h"

#include <cmath>

Monster::Monster(const std::string &propertyName) :
  Mob             (propertyName),

  nextMoveT       (0.0),
  moveTarget      (0, 0, 0),
  isMoveTargetValid (false),

  attackTarget    (InvalidID),
  nextAttackT     (0.0)
{}

Monster::Monster(const std::string &type, Deserializer &deser) :
  Mob             (type, deser) {
  deser >> nextMoveT >> moveTarget >> isMoveTargetValid;
  deser >> attackTarget >> nextAttackT;
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
    isMoveTargetValid = false;
    return;
  }

  //Log("Monster::Think: %u %s\n", this->GetId(), this->GetName().c_str());

  if (this->attackTarget != InvalidID) {
    Log("  I have a target %u\n", this->attackTarget);

    Entity *enemy = state.GetEntity(this->attackTarget);

    if (enemy) {
      Log("  It is valid: %p (%s)\n", enemy, enemy->GetName().c_str());

      float dist = (enemy->GetPosition() - GetPosition()).GetMag();

      if (dist > this->properties->aggroRangeFar) {
        // out of range? -> unset attack target
        this->attackTarget = InvalidID;

        Log("  It is out of range, bummer...\n");

      } else if (!CanSee(state, enemy->GetPosition())) {
        // not visible? -> set move target to last known location, unset attack target
        this->isMoveTargetValid = true;
        this->moveTarget = enemy->GetPosition();
        this->attackTarget = InvalidID;

        Log("  I can not see it...\n");
      } else if (dist < this->properties->meleeAttackRange &&
                 dist >= this->properties->keepDistance &&
                 state.GetGame().GetTime() > nextAttackT) {
        Log("  I chose to attack it...\n");
        this->sprite.StartAnim(this->properties->attackAnim);

        if (this->properties->attackForwardStep) {
          this->AddVelocity((enemy->GetPosition() - GetPosition()).Horiz().Normalize() * this->properties->attackForwardStep);
        }

        if (this->attackItem) {
          this->attackItem->UseOnEntity(state, *this, this->attackTarget);
        }

        this->velocity.y += this->properties->attackJump;
        nextAttackT = state.GetGame().GetTime() + this->properties->attackInterval;
      }

    } else {
      Log("  It is invalid...\n");
      this->attackTarget = InvalidID;
    }
  }

  if (this->attackTarget == InvalidID && this->properties->aggressive) {
    // Log("  I don't have a target, but I am aggressive and looking for one...\n");
    // find enitites in aggroRange
    std::vector<ID> ents = state.FindEntities(AABB(aabb.center, Vector3(this->properties->aggroRangeNear)));
    Entity *enemy = nullptr;

    for (size_t e : ents) {
      Entity *entity = state.GetEntity(e);

      if (!entity) continue;

      // suitable? -> set attack target
      if (entity->GetProperties()->name == "player" && CanSee(state, entity->GetPosition())) {
        enemy = entity;
        Log("  Found one: %u %s\n", enemy->GetId(), enemy->GetName().c_str());
        break;
      }
    }

    if (enemy) {
      this->attackTarget = enemy->GetId();
    }
  }

  if (this->attackTarget != InvalidID) {
    // walk toward enemy
    Entity *enemy = state.GetEntity(this->attackTarget);

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
      this->attackTarget = InvalidID;
    }

  } else if (this->attackTarget == InvalidID && this->properties->moveInterval != 0) {
    // walk around a bit
    if (state.GetGame().GetTime() > nextMoveT) {
      nextMoveT += this->properties->moveInterval;
      moveTarget = aabb.center + (Vector3(state.GetRandom().Float(), state.GetRandom().Float(), state.GetRandom().Float())) * 4.0;
      isMoveTargetValid = true;
    }

    Vector3 tmove = (moveTarget - aabb.center).Horiz();

    if (tmove.GetMag() < 0.1) {
      isMoveTargetValid = false;

    } else {
      forward = (moveTarget-aabb.center).Normalize();
    }

    if (isMoveTargetValid) move = tmove.Normalize() * this->properties->maxSpeed;

    //Log("  Moving toward %f %f %f...\n", moveTarget.x, moveTarget.y, moveTarget.z);
  }
}

void
Monster::OnCollide(RunningState &state, Entity &other) {
  Mob::OnCollide(state, other);

  if (this->IsSolid()) return;

  if (this->properties->attackForwardStep && other.GetId() == this->attackTarget) {
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
    this->attackTarget = info.dealerId;
  }
}

void
Monster::Serialize(Serializer &ser) const {
  Mob::Serialize(ser);

  ser << nextMoveT << moveTarget << isMoveTargetValid;
  ser << attackTarget << nextAttackT;
}
