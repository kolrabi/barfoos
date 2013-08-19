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
  moveTarget      (0,0,0),
  isMoveTargetValid (false),

  attackTarget    (InvalidID),
  nextAttackT     (0.0)
{}

Monster::Monster(const std::string &type, Deserializer &deser) : 
  Mob             (type, deser)
{
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

  if (this->attackTarget != InvalidID) {
    Entity *enemy = state.GetEntity(this->attackTarget);
    if (enemy) {
      float dist = (enemy->GetPosition() - GetPosition()).GetMag();
      if (dist > this->properties->aggroRangeFar) {
        // out of range? -> unset attack target
        this->attackTarget = InvalidID;
      } else if (!CanSee(state, enemy->GetPosition())) {
        // not visible? -> set move target to last known location, unset attack target
        this->isMoveTargetValid = true;
        this->moveTarget = enemy->GetPosition();
        this->attackTarget = InvalidID;
      } else if (dist < this->properties->meleeAttackRange && state.GetGame().GetTime() > nextAttackT) {
        this->sprite.StartAnim(this->properties->attackAnim);

        if (this->properties->attackForwardStep) {
          this->AddVelocity((enemy->GetPosition() - GetPosition()).Horiz().Normalize() * this->properties->attackForwardStep);
        } else {
          if (this->attackItem) this->attackItem->UseOnEntity(state, *this, this->attackTarget);
        }
        this->velocity.y += this->properties->attackJump;
        nextAttackT = state.GetGame().GetTime() + this->properties->attackInterval;
      }
    } else {
      this->attackTarget = InvalidID;
    }
  }

  if (this->attackTarget == InvalidID && this->properties->aggressive) {
    // find enitites in aggroRange
    std::vector<ID> ents = state.FindEntities(AABB(aabb.center, Vector3(this->properties->aggroRangeNear)));
    Entity *enemy = nullptr;
    for (size_t e:ents) {
      Entity *entity = state.GetEntity(e);
      if (!entity) continue;

      // suitable? -> set attack target
      if (entity->GetProperties()->name == "player" && CanSee(state, entity->GetPosition())) {
        enemy = entity;
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
      Vector3 dhoriz = (enemy->GetPosition() - GetPosition()).Horiz();
      if (dhoriz.GetMag() > this->properties->meleeAttackRange) {
        move = dhoriz.Normalize() * this->properties->maxSpeed;
      } else {
        move = Vector3();
      }
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
      angles.x = std::atan2(tmove.z, tmove.x);
    }
    if (isMoveTargetValid) move = tmove.Normalize() * this->properties->maxSpeed;
  }
}

void
Monster::OnCollide(RunningState &state, Entity &other) {
  Mob::OnCollide(state, other);
  
  if (this->IsSolid()) return;

  if (this->properties->attackForwardStep && other.GetId() == this->attackTarget) {
     this->AddVelocity(-(other.GetPosition() - GetPosition()).Horiz().Normalize() * this->properties->attackForwardStep);
     if (this->attackItem) this->attackItem->UseOnEntity(state, *this, other.GetId());
     return;
  }

  Vector3 d = this->GetAABB().center - other.GetAABB().center;
  Vector3 f = d * (this->properties->mass * other.GetProperties()->mass / (1+d.GetSquareMag()));
  this->ApplyForce(state, f);
}

void
Monster::Serialize(Serializer &ser) const {
  Mob::Serialize(ser);

  ser << nextMoveT << moveTarget << isMoveTargetValid;
  ser << attackTarget << nextAttackT;
}
