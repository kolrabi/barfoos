#include "mob.h"
#include "world.h"
#include "cell.h"
#include "util.h"
#include "runningstate.h"
#include "item.h"

#include "serializer.h"
#include "deserializer.h"

#include <cmath>

Mob::Mob(const std::string &propertyName) : 
  Entity          (propertyName),
  
  velocity        (0, 0, 0),
  move            (0, 0, 0),
  
  wantJump        (false),
  lastJumpT       (0.0),
  
  onGround        (false),
  inWater         (false),
  underWater      (false),
  noclip          (false),
  sneak           (false),
  
  nextMoveT       (0.0),
  moveTarget      (0,0,0),
  validMoveTarget (false),
  
  attackTarget    (~0UL),
  nextAttackT     (0.0),
  
  headCell        (nullptr),
  footCell        (nullptr),
  groundCell      (nullptr)
{}

Mob::Mob(const std::string &type, Deserializer &deser) : Entity(type, deser),
  wantJump        (false),
  
  inWater         (false),
  underWater      (false),
  
  headCell        (nullptr),
  footCell        (nullptr),
  groundCell      (nullptr)
{
  deser >> velocity;
  deser >> lastJumpT;
  deser >> onGround;
  deser >> noclip;
  deser >> sneak;
  
  deser >> nextMoveT >> moveTarget >> validMoveTarget;
  
  deser >> attackTarget >> nextAttackT;
}

Mob::~Mob() {
}

void 
Mob::Start(RunningState &state, size_t id) {
  Entity::Start(state, id);
  if (properties->attackItem != "") {
    this->attackItem = std::shared_ptr<Item>(new Item(properties->attackItem));
    this->attackItem->Update(state);
  }
}

void 
Mob::Continue(RunningState &state, size_t id) {
  Entity::Continue(state, id);
  if (properties->attackItem != "") {
    this->attackItem = std::shared_ptr<Item>(new Item(properties->attackItem));
    this->attackItem->Update(state);
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
  
  if (this->attackItem) this->attackItem->Update(state);

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

  if (noclip) {
    if (wantJump) {
      tvy = 3;
      wantJump = false;
    } else if (sneak) {
      tvy = -3;
      sneak = false;
    } else {
      tvy = 0;
    }
  } else if (inWater) { 
    if (wantJump) {
      tvy = 3 * friction;
      wantJump = false;
    } else {
      tvy = -3 * friction;
    }
  } else {
    tvy = velocity.y -= gravity;
    if (wantJump && onGround) {
      if (game.GetTime() - lastJumpT > 0.5) {
        tvy = velocity.y = 8;
        lastJumpT = game.GetTime();
      }
    }
    wantJump = false;
  }
  
  velocity.x += (tvx-velocity.x) * groundFriction * deltaT * 10;
  velocity.y += (tvy-velocity.y) * groundFriction * deltaT * 10;
  velocity.z += (tvz-velocity.z) * groundFriction * deltaT * 10;
  

  if (!this->properties->noFriction) {
    velocity.x = velocity.x * friction;
    if (this->inWater) velocity.y = velocity.y * friction;
    velocity.z = velocity.z * friction;
  }

  // move
  World &world = state.GetWorld();
  uint8_t axis, axis2;
  bool movingDown = velocity.y <= 0;
  Cell *cell = nullptr;
  Side side;

  if (noclip) {
    aabb.center = aabb.center + velocity * deltaT;
  } else if (movingDown) {
    // when moving down and pushing use step height
    Vector3 oldCenter = aabb.center;
    
    Vector3 step(0, move.GetMag()!=0 ? this->properties->stepHeight : 0, 0);
    Vector3 org = aabb.center + velocity.Horiz() * deltaT;
    bool downStep;
    
    aabb.center = world.MoveAABB(aabb, aabb.center + step);
    aabb.center = world.MoveAABB(aabb, aabb.center + velocity.Horiz()*deltaT, axis, &cell, &side);
    aabb.center = world.MoveAABB(aabb, aabb.center - step*1.25 + velocity.Vert()*deltaT, axis2, cell ? nullptr : &cell, &side);
    downStep = axis2 & Axis::Y;
    org.y = aabb.center.y;
    axis |= axis2;
    aabb.center = world.MoveAABB(aabb, org, axis2);
    axis |= axis2;
    if (!downStep) aabb.center = aabb.center + step*0.25;
    
    Vector3 newCenter = aabb.center;
    aabb.center = oldCenter;
    aabb.center = state.MoveAABB(aabb, newCenter, axis2);
    axis |= axis2;
  } else {
    onGround = false;
    Vector3 newCenter = world.MoveAABB(aabb, aabb.center + velocity*deltaT, axis, &cell, &side);
    aabb.center = state.MoveAABB(aabb, newCenter, axis2);
    axis |= axis2;
  }
  this->smoothPosition = this->aabb.center;
  
  if (cell && !this->properties->nocollideCell) {
    ((Entity*)this)->OnCollide(state, *cell, side);
  }
  
  // jump out of water
  if (axis & Axis::Horizontal && (inWater || validMoveTarget) && move.GetMag() != 0 && !noclip && !sneak) {
    wantJump = true;
  }

  // fall damage  
  if (axis & Axis::Y) {
    if (velocity.y < -15) {
      AddHealth(state, HealthInfo( (velocity.y+15)/5, HealthType::Falling));
    }
    velocity.y = 0;
    onGround |= movingDown;
  }
  
  IVector3 footPos(aabb.center.x, aabb.center.y - aabb.extents.y + this->properties->stepHeight, aabb.center.z);
  IVector3 headPos(aabb.center.x, aabb.center.y + aabb.extents.y, aabb.center.z);
  
  footCell = &world.GetCell(footPos);
  headCell = &world.GetCell(headPos);

  if (onGround) {
    groundCell = &world.GetCell(footPos[Side::Down]);
  } else {
    groundCell = nullptr;
  }

  underWater = headCell->GetInfo().flags & CellFlags::Liquid;
  if (!noclip) this->SetInLiquid(footCell->GetInfo().flags & (CellFlags::Liquid | CellFlags::Ladder));

  if (footCell->GetInfo().lavaDamage) {
    this->AddHealth(state, HealthInfo( -footCell->GetInfo().lavaDamage * deltaT, HealthType::Lava));
  } else if (headCell->GetInfo().lavaDamage) {
    this->AddHealth(state, HealthInfo( -headCell->GetInfo().lavaDamage * deltaT, HealthType::Lava));
  }

  if (this->IsDead()) {
    this->wantJump = false;
  }
}

void
Mob::Think(RunningState &state) {  
  Entity::Think(state);

  if (this->IsDead()) {
    validMoveTarget = false;
    return;
  }
  
  if (this->attackTarget != ~0UL) {
    Entity *enemy = state.GetEntity(this->attackTarget);
    if (enemy) {
      float dist = (enemy->GetPosition() - GetPosition()).GetMag();
      if (dist > this->properties->aggroRangeFar) {
        // out of range? -> unset attack target
        this->attackTarget = ~0UL;
      } else if (!CanSee(state, enemy->GetPosition())) {
        // not visible? -> set move target to last known location, unset attack target
        this->validMoveTarget = true;
        this->moveTarget = enemy->GetPosition();
        this->attackTarget = ~0UL;
      } else if (dist < this->properties->meleeAttackRange && state.GetGame().GetTime() > nextAttackT) {
        this->sprite.StartAnim(this->properties->attackAnim);
        nextAttackT += this->properties->attackInterval;
        
        if (this->attackItem) this->attackItem->UseOnEntity(state, *this, this->attackTarget);
      }
    } else {
      this->attackTarget = ~0UL;
    }
  }
  
  if (this->attackTarget == ~0UL && this->properties->aggressive) {
    // find enitites in aggroRange
    std::vector<size_t> ents = state.FindEntities(AABB(aabb.center, Vector3(this->properties->aggroRangeNear)));
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
  
  if (this->attackTarget != ~0UL) {
    // walk toward enemy
    Entity *enemy = state.GetEntity(this->attackTarget);
    if (enemy) {
      move = (enemy->GetPosition() - GetPosition()).Horiz().Normalize() * this->properties->maxSpeed;
    } else {
      this->attackTarget = ~0UL;
    }
  } else if (this->attackTarget == ~0UL && this->properties->moveInterval != 0) {
    // walk around a bit
    if (state.GetGame().GetTime() > nextMoveT) {
      nextMoveT += this->properties->moveInterval;
      moveTarget = aabb.center + (Vector3(state.GetRandom().Float(), state.GetRandom().Float(), state.GetRandom().Float())) * 4.0;
      validMoveTarget = true;
    }
    
    Vector3 tmove = (moveTarget - aabb.center).Horiz();
    if (tmove.GetMag() < 0.1) validMoveTarget = false;
    if (validMoveTarget) move = tmove.Normalize() * this->properties->maxSpeed;
  }
}

void
Mob::SetInLiquid(bool inLiquid) {
  if (inLiquid == this->inWater) return;
  this->inWater = inLiquid;
  if (inLiquid) {
    // enter liquid
  } else {
    // exit liquid
  }
}

void
Mob::Die(RunningState &state, const HealthInfo &info) {
  Entity::Die(state, info);
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
  if (sneak) mod *= 0.5;
  if (this->footCell) mod *= this->footCell->GetInfo().speedModifier;
  return mod * (1.0 + GetEffectiveStats().agi * Const::WalkSpeedFactorPerAGI);
}

void
Mob::Serialize(Serializer &ser) const {
  Entity::Serialize(ser);
  
  ser << velocity;
  ser << lastJumpT;
  ser << onGround;
  ser << noclip;
  ser << sneak;
  
  ser << nextMoveT << moveTarget << validMoveTarget;
  
  ser << attackTarget << nextAttackT;
}
