#include "mob.h"
#include "world.h"
#include "cell.h"
#include "util.h"
#include "runningstate.h"
#include "item.h"
#include "audio.h"

#include "serializer.h"
#include "deserializer.h"

#include <cmath>

Mob::Mob(const std::string &propertyName) :
  Entity          (propertyName),

  lastKnownGoodPos(0, 0, 0),
  velocity        (0, 0, 0),
  move            (0, 0, 0),

  doesWantJump        (false),
  lastJumpT       (0.0),

  isOnGround        (false),
  isInLiquid         (false),
  isSubmerged      (false),
  isNoclip          (false),
  isSneaking           (false),

  headCell        (nullptr),
  footCell        (nullptr),
  groundCell      (nullptr)
{}

Mob::Mob(const std::string &type, Deserializer &deser) : Entity(type, deser),
  doesWantJump        (false),

  isInLiquid         (false),
  isSubmerged      (false),

  headCell        (nullptr),
  footCell        (nullptr),
  groundCell      (nullptr)
{
  deser >> velocity;
  deser >> lastJumpT;
  deser >> isOnGround;
  deser >> isNoclip;
  deser >> isSneaking;
}

Mob::~Mob() {
}

void
Mob::Start(RunningState &state, ID id) {
  Entity::Start(state, id);

  if (this->properties->createBubbles) {
    this->regulars["bubble"] = Regular(0.2, [&]() {
      if (this->footCell && this->footCell->IsLiquid())
        state.SpawnInAABB("particle.bubble", this->footCell->GetAABB());
    });
  }
}

void
Mob::Continue(RunningState &state, ID id) {
  Entity::Continue(state, id);

  if (this->properties->createBubbles) {
    this->regulars["bubble"] = Regular(0.2, [&]() {
      if (this->footCell && this->footCell->IsLiquid())
        state.SpawnInAABB("particle.bubble", this->footCell->GetAABB());
    });
  }
}

void
Mob::Update(RunningState &state) {
  Entity::Update(state);

  Game &game = state.GetGame();
  float deltaT = game.GetDeltaT();

  if (this->IsDead()) {
    this->move = Vector3();
  }

  if (state.GetWorld().IsAABBSolid(this->aabb)) {
    //if (!this->isNoclip) this->aabb.center = this->lastKnownGoodPos;
  } else {
    this->lastKnownGoodPos = this->aabb.center;
  }

  // clip move speed
  float speed = move.GetMag();
  float maxSpeed = this->properties->maxSpeed * this->GetMoveModifier() * this->GetEffectiveStats().walkSpeed;
  if (speed > maxSpeed) move = move * (maxSpeed/speed);

  float gravity = 3 * 9.81 * deltaT * this->properties->gravity;
  float footFriction = 1.0 / (this->footCell ? this->footCell->GetInfo().speedModifier : 1.0);
  float groundFriction = this->groundCell ? this->groundCell->GetInfo().friction : 0.1;
  float friction = 1.0 / (1.0+deltaT * 10 * footFriction * groundFriction);

  float tvx = std::abs(move.x) > std::abs(velocity.x) ? move.x : velocity.x;
  float tvy = velocity.y;
  float tvz = std::abs(move.z) > std::abs(velocity.z) ? move.z : velocity.z;

  if (isNoclip) {
    if (doesWantJump) {
      tvy = 3;
      doesWantJump = false;
    } else if (isSneaking) {
      tvy = -3;
      isSneaking = false;
    } else {
      tvy = 0;
    }
  } else if (isInLiquid) {
    if (doesWantJump || properties->swim) {
      tvy = 3 * friction;
      doesWantJump = false;
    } else {
      tvy = -3 * friction * this->properties->gravity;
    }
  } else {
    tvy = velocity.y -= gravity;
    if (doesWantJump && isOnGround) {
      if (game.GetTime() - lastJumpT > 0.5) {
        tvy = velocity.y = properties->jumpSpeed;
        state.GetGame().GetAudio().PlaySound("jump", this->GetSmoothPosition());
        lastJumpT = game.GetTime();
      }
    }
    doesWantJump = false;
  }

  velocity.x += (tvx-velocity.x) * groundFriction * deltaT * 10;
  velocity.y += (tvy-velocity.y) * groundFriction * deltaT * 10;
  velocity.z += (tvz-velocity.z) * groundFriction * deltaT * 10;

  if (!this->properties->noFriction) {
    velocity.x = velocity.x * friction;
    if (this->isInLiquid) velocity.y = velocity.y * friction;
    velocity.z = velocity.z * friction;
  }

  // move
  World &world = state.GetWorld();
  uint8_t axesTotal = 0;
  bool isMovingDown = velocity.y <= 0;
  Cell *cell = nullptr;
  Side side;

  if (isNoclip) {
    aabb.center = aabb.center + velocity * deltaT;
  } else if (isMovingDown) {
    Vector3 oldCenter = aabb.center;
    Vector3 org = aabb.center + velocity.Horiz() * deltaT;
    uint8_t axesForward = 0, axesDown = 0;

    // when moving down and pushing use step height
    Vector3 step(0, move.GetMag()!=0 ? this->properties->stepHeight : 0, 0);

    // move up, forward, down
    aabb.center = world.MoveAABB(aabb, aabb.center + step);
    aabb.center = world.MoveAABB(aabb, aabb.center + velocity.Horiz()*deltaT, axesForward, &cell, &side);
    aabb.center = world.MoveAABB(aabb, aabb.center - step*1.25 + velocity.Vert()*deltaT, axesDown, cell ? nullptr : &cell, &side);

    axesTotal |= axesForward | axesDown;

    bool didHitFloor = axesDown & Axis::Y;

    org.y = aabb.center.y;
    aabb.center = world.MoveAABB(aabb, org, axesDown);
    axesTotal |= axesDown;

    if (!didHitFloor) aabb.center = aabb.center + step*0.25;

    // check collision with entities
    Vector3 newCenter = aabb.center;
    uint8_t axesEntities = 0;
    aabb.center = oldCenter;
    aabb.center = state.MoveAABB(aabb, newCenter, axesEntities);
    axesTotal |= axesEntities;
  } else {
    // moving up
    isOnGround = false;
    uint8_t axesWorld = 0, axesEntities = 0;
    Vector3 newCenter = world.MoveAABB(aabb, aabb.center + velocity*deltaT, axesWorld, &cell, &side);
    aabb.center = state.MoveAABB(aabb, newCenter, axesEntities);
    axesTotal |= axesWorld | axesEntities;
  }

  bool didCollideHorizontal = axesTotal & Axis::Horizontal;
  bool didCollideVertical   = axesTotal & Axis::Y;

  // update new target position for smoothing
  this->smoothPosition = this->aabb.center;

  if (cell && !this->properties->nocollideCell) {
    Entity *e = this; // WTF? GCC bug?
    e->OnCollide(state, *cell, side);
  }

  // jump out of water
  if (didCollideHorizontal && isInLiquid && move.GetMag() != 0 && !isNoclip && !isSneaking) {
    doesWantJump = true;
  }

  // open doors
  if (didCollideHorizontal && cell && properties->onCollideUseCell) {
    cell->OnUse(state, *this);
  }

  // fall damage
  if (didCollideVertical) {
    if (velocity.y < -15) {
      AddHealth(state, HealthInfo( (velocity.y+15)/5, HealthType::Falling));
    }
    velocity.y = 0;
    isOnGround |= isMovingDown;
  }

  IVector3 footPos(aabb.center.x, aabb.center.y - aabb.extents.y, aabb.center.z);
  IVector3 headPos(aabb.center.x, aabb.center.y + aabb.extents.y, aabb.center.z);

  footCell = &world.GetCell(footPos);
  headCell = &world.GetCell(headPos);

  Cell *newGroundCell = nullptr;
  if (isOnGround) {
    newGroundCell = &world.GetCell(footPos[Side::Down]);
  }

  if (newGroundCell != groundCell) {
    if (groundCell && !properties->noStep) groundCell->OnStepOff(state, *this);
    groundCell = newGroundCell;
    if (groundCell && !properties->noStep) groundCell->OnStepOn(state, *this);
  }

  isSubmerged = headCell->IsLiquid();
  if (!isNoclip) this->SetInLiquid(footCell->GetInfo().flags & (CellFlags::Liquid | CellFlags::Ladder));

  if (footCell->GetInfo().lavaDamage) {
    this->AddHealth(state, HealthInfo( -footCell->GetInfo().lavaDamage * deltaT, HealthType::Lava));
  } else if (headCell->GetInfo().lavaDamage) {
    this->AddHealth(state, HealthInfo( -headCell->GetInfo().lavaDamage * deltaT, HealthType::Lava));
  }

  if (this->IsDead()) {
    this->doesWantJump = false;
  }
}

void
Mob::Think(RunningState &state) {
  Entity::Think(state);

  if (this->IsDead()) return;
}

void
Mob::SetInLiquid(bool inLiquid) {
  if (inLiquid == this->isInLiquid) return;
  this->isInLiquid = inLiquid;
  /*
  if (inLiquid) {
    // play splash sound
  } else {
    // play splash sound
  }
  */
}

void
Mob::OnCollide(RunningState &state, Entity &other) {
  if (this->IsSolid()) return;

  Vector3 d = this->GetAABB().center - other.GetAABB().center;
  Vector3 f = d * (this->properties->mass * other.GetProperties()->mass / (1+d.GetSquareMag()));
  this->ApplyForce(state, f);
}

void
Mob::ApplyForce(RunningState &state, const Vector3 &f) {
  velocity = velocity + f * (state.GetGame().GetDeltaT() / this->properties->mass);
}

float
Mob::GetMoveModifier() const {
  float mod = 1.0;
  if (isSneaking) mod *= 0.5;
  if (this->footCell) mod *= this->footCell->GetInfo().speedModifier;
  return mod * (1.0 + GetEffectiveStats().agi * Const::WalkSpeedFactorPerAGI);
}

void
Mob::Serialize(Serializer &ser) const {
  Entity::Serialize(ser);

  ser << velocity;
  ser << lastJumpT;
  ser << isOnGround;
  ser << isNoclip;
  ser << isSneaking;
}
