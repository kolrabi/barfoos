#include "common.h"

#include "audio/audio.h"
#include "game/entities/mob.h"
#include "game/gamestates/running/runningstate.h"
#include "game/items/item.h"
#include "game/world/cells/cell.h"
#include "game/world/world.h"
#include "util/util.h"

Mob::Mob(const std::string &propertyName) :
  Entity          (propertyName),

  move            (0, 0, 0),
  doesWantJump        (false),

  headCell        (nullptr),
  footCell        (nullptr),
  groundCell      (nullptr)
{
  this->proto.set_spawn_class(uint32_t(SpawnClass::MobClass));
  this->proto.mutable_mob();
  this->SetNoClip(false);
  this->SetSneaking(false);
  this->SetInLiquid(false);
  this->SetLastJumpTime(0.0f);
}

Mob::Mob(const Entity_Proto &proto) :
  Entity          (proto),

  move            (0, 0, 0),
  doesWantJump        (false),

  headCell        (nullptr),
  footCell        (nullptr),
  groundCell      (nullptr)
{
}

Mob::~Mob() {
}

void
Mob::Start(RunningState &state, ID id) {
  Entity::Start(state, id);

  if (this->properties->createBubbles) {
    this->regulars["bubble"] = Regular(0.2, [&]() {
      if (this->footCell && this->footCell->IsLiquid())
        state.SpawnInAABB("particle.bubble", this->footCell->GetAABB(), Vector3());
    });
  }
}

void
Mob::Continue(RunningState &state, ID id) {
  Entity::Continue(state, id);

  if (this->properties->createBubbles) {
    this->regulars["bubble"] = Regular(0.2, [&]() {
      if (this->footCell && this->footCell->IsLiquid())
        state.SpawnInAABB("particle.bubble", this->footCell->GetAABB(), Vector3());
    });
  }
}

void
Mob::Update(RunningState &state) {
  Entity::Update(state);

  PROFILE();

  Game &game = state.GetGame();
  float deltaT = game.GetDeltaT();

  if (this->IsDead()) {
    this->move = Vector3();
  }

  // clip move speed
  float speed = move.GetMag();
  float maxSpeed = this->properties->maxSpeed * this->GetMoveModifier() * this->GetEffectiveStats().GetWalkSpeed();
  if (speed > maxSpeed) move = move * (maxSpeed/speed);

  float gravity = 3 * 9.81 * deltaT * this->properties->gravity;
  float footFriction = 1.0 / (this->footCell ? this->footCell->GetInfo().speedModifier : 1.0);
  float groundFriction = this->groundCell ? this->groundCell->GetInfo().friction : 0.1;
  float friction = 1.0 / (1.0+deltaT * 10 * footFriction * groundFriction);

  Vector3 velocity = this->GetVelocity();

  float tvx = std::abs(move.x) > std::abs(velocity.x) ? move.x : velocity.x;
  float tvy = velocity.y;
  float tvz = std::abs(move.z) > std::abs(velocity.z) ? move.z : velocity.z;

  if (this->IsNoclip()) {
    if (doesWantJump) {
      tvy = 3;
      doesWantJump = false;
    } else if (this->IsSneaking()) {
      tvy = -3;
      SetSneaking(false);
    } else {
      tvy = 0;
    }
  } else if (this->IsInLiquid()) {
    if (doesWantJump || properties->swim) {
      tvy = 3 * friction;
      doesWantJump = false;
    } else {
      tvy = -3 * friction * this->properties->gravity;
    }
  } else {
    tvy = velocity.y -= gravity;
    if (doesWantJump && this->IsOnGround()) {
      if (game.GetTime() - this->GetLastJumpTime() > 0.5) {
        tvy = velocity.y = properties->jumpSpeed;
        this->PlaySound(state, "jump");
        this->SetLastJumpTime(game.GetTime());
      }
    }
    doesWantJump = false;
  }

  velocity.x += (tvx-velocity.x) * groundFriction * deltaT * 10;
  velocity.y += (tvy-velocity.y) * groundFriction * deltaT * 10;
  velocity.z += (tvz-velocity.z) * groundFriction * deltaT * 10;

  if (!this->properties->noFriction) {
    velocity.x = velocity.x * friction;
    if (this->IsInLiquid()) velocity.y = velocity.y * friction;
    velocity.z = velocity.z * friction;
  }

  // move
  World &world = state.GetWorld();
  uint8_t axesTotal = 0;
  bool isMovingDown = velocity.y <= 0;
  Cell *cell = nullptr;
  Side side;

  if (this->IsNoclip()) {
    aabb.center = aabb.center + velocity * deltaT;
  } else if (isMovingDown) {
    Vector3 oldCenter = aabb.center;
    Vector3 org = aabb.center + velocity.Horiz() * deltaT;
    uint8_t axesForward = 0, axesDown = 0;

    // when moving down and pushing use step height
    Vector3 step(0, (move.GetMag()!=0 && !this->IsSneaking()) ? this->properties->stepHeight : 0, 0);

    // move up, forward, down
    aabb.center = world.MoveAABB(aabb, aabb.center + step);
    aabb.center = world.MoveAABB(aabb, aabb.center + velocity.Horiz()*deltaT, axesForward, &cell, &side, this->IsSneaking() && this->IsOnGround());
    aabb.center = world.MoveAABB(aabb, aabb.center - step*1.25 + velocity.Vert()*deltaT, axesDown, cell ? nullptr : &cell, &side);

    axesTotal |= axesForward | axesDown;

    bool didHitFloor = axesDown & Axis::Y;

    this->SetOnGround(didHitFloor);

    org.y = aabb.center.y;
    aabb.center = world.MoveAABB(aabb, org, axesDown, nullptr, nullptr, this->IsSneaking() && this->IsOnGround());
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
    this->SetOnGround(false);
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
  if (didCollideHorizontal && this->IsInLiquid() && move.GetMag() != 0 && !this->IsNoclip() && !this->IsSneaking()) {
    doesWantJump = true;
  }

  // open doors
  if (didCollideHorizontal && cell && properties->onCollideUseCell) {
    cell->OnUse(state, *this);
  }

  // fall damage
  if (didCollideVertical) {
    if (velocity.y < -6) {
      this->PlaySound(state, "land");
    }
    if (velocity.y < -15) {
      this->PlaySound(state, "land.hard");
      AddHealth(state, HealthInfo( (velocity.y+15)/5, HealthType::Falling));
    }
    velocity.y = 0;
    this->SetOnGround(this->IsOnGround() || isMovingDown);
  }

  this->SetVelocity(velocity);

  IVector3 footPos(aabb.center.x, aabb.center.y - aabb.extents.y, aabb.center.z);
  IVector3 headPos(aabb.center.x, aabb.center.y + aabb.extents.y, aabb.center.z);

  footCell = &world.GetCell(footPos);
  headCell = &world.GetCell(headPos);

  Cell *newGroundCell = nullptr;
  if (this->IsOnGround()) {
    newGroundCell = &world.GetCell(footPos[Side::Down]);
  }

  if (newGroundCell != groundCell) {
    if (groundCell && !properties->noStep) groundCell->OnStepOff(state, *this);
    groundCell = newGroundCell;
    if (groundCell && !properties->noStep) groundCell->OnStepOn(state, *this);
  }

  this->SetSubmerged(headCell->IsLiquid());
  if (!this->IsNoclip()) this->SetInLiquid(footCell->GetInfo().flags & (CellFlags::Liquid | CellFlags::Ladder));

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
  this->proto.mutable_mob()->set_is_in_liquid(inLiquid);
  if (inLiquid == this->IsInLiquid()) return;
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
  this->AddImpulse(f * state.GetGame().GetDeltaT());
}

float
Mob::GetMoveModifier() const {
  float mod = 1.0;
  if (this->IsSneaking()) mod *= 0.5;
  if (this->footCell) mod *= this->footCell->GetInfo().speedModifier;
  return mod * (1.0 + GetEffectiveStats().GetAgility() * Const::WalkSpeedFactorPerAGI);
}

Cell *
Mob::GetSelection(RunningState &state, float range, const std::shared_ptr<Item> &item, Side &selectedCellSide, ID &entityId) {
  Vector3 dir = this->GetForward();
  Vector3 pos = this->GetSmoothEyePosition();

  auto entitiesInRange = state.FindEntities(pos, range);

  float hitDist = range;
  float dist  = range;
  Vector3 hitPos;

  entityId = InvalidID;

  // check entities in range
  for (auto id : entitiesInRange) {
    Entity *entity = state.GetEntity(id);
    if (!entity || entity == this) continue;
    if (entity->IsDead()) continue;
    if (entity->GetProperties()->nohit || (item && entity->GetProperties()->noItemUse)) continue;

    if (entity->GetAABB().Ray(pos, dir, hitDist, hitPos)) {
      if (hitDist < dist) {
        dist = hitDist;
        entityId = id;
      }
    }
  }

  // check cells
  size_t flags = CellFlags::Pickable;
  if (item && item->GetProperties().pickLiquid) flags |= CellFlags::Liquid;

  Cell &cell = state.GetWorld().CastRayCell(pos, dir, hitDist, selectedCellSide, flags);
  if (hitDist < dist) {
    entityId = InvalidID;
    return &cell;
  } else {
    return nullptr;
  }
}

bool
Mob::HasLearntSpell(const std::string &name) const {
  return std::find(
    this->proto.mob().learnt_spells().begin(), 
    this->proto.mob().learnt_spells().end(), 
    name
  ) != this->proto.mob().learnt_spells().end();
}

void
Mob::LearnSpell(const std::string &name) {
  Log("%s %d\n", __PRETTY_FUNCTION__, __LINE__);
  if (!HasLearntSpell(name)) {
    Log("%s %d\n", __PRETTY_FUNCTION__, __LINE__);
    this->proto.mutable_mob()->add_learnt_spells(name);
  }
  Log("%s %d\n", __PRETTY_FUNCTION__, __LINE__);
}

