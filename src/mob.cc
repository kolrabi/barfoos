#include "mob.h"
#include "world.h"
#include "cell.h"
#include "util.h"
#include "game.h"

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
  
  headCell        (nullptr),
  footCell        (nullptr),
  groundCell      (nullptr)
{}

Mob::~Mob() {
}

void 
Mob::Start(Game &game, size_t id) {
  Entity::Start(game, id);
  this->sprite.t = game.GetRandom().Float01();
}

void 
Mob::Update(Game &game) {
  Entity::Update(game);

  float deltaT = game.GetDeltaT();
  
  // clip move speed
  float speed = move.GetMag();
  float maxSpeed = this->properties->maxSpeed * this->GetMoveModifier();
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
  World &world = game.GetWorld();
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
    aabb.center = game.MoveAABB(aabb, newCenter, axis2);
    axis |= axis2;
  } else {
    onGround = false;
    Vector3 newCenter = world.MoveAABB(aabb, aabb.center + velocity*deltaT, axis, &cell, &side);
    aabb.center = game.MoveAABB(aabb, newCenter, axis2);
    axis |= axis2;
  }
  this->smoothPosition = this->aabb.center;
  
  if (cell && !this->properties->nocollideCell) {
    ((Entity*)this)->OnCollide(game, *cell, side);
  }
  
  // jump out of water
  if (axis & Axis::Horizontal && (inWater || validMoveTarget) && move.GetMag() != 0 && !noclip && !sneak) {
    wantJump = true;
  }

  // fall damage  
  if (axis & Axis::Y) {
    if (velocity.y < -15) {
      AddHealth(game, HealthInfo( (velocity.y+15)/5, HealthType::Falling));
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
    this->AddHealth(game, HealthInfo( -footCell->GetInfo().lavaDamage * deltaT, HealthType::Lava));
  } else if (headCell->GetInfo().lavaDamage) {
    this->AddHealth(game, HealthInfo( -headCell->GetInfo().lavaDamage * deltaT, HealthType::Lava));
  }
}

void
Mob::Think(Game &game) {  
  Entity::Think(game);
  
  // walk around a bit
  if (this->properties->moveInterval != 0) {
    if (game.GetTime() > nextMoveT) {
      nextMoveT += this->properties->moveInterval;
      moveTarget = aabb.center + (Vector3(game.GetRandom().Float(), game.GetRandom().Float(), game.GetRandom().Float())) * 4.0;
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
Mob::Die(Game &game, const HealthInfo &info) {
  Entity::Die(game, info);
}

void
Mob::OnCollide(Game &game, Entity &other) {
  if (this->IsSolid()) return;
  Vector3 d = this->GetAABB().center - other.GetAABB().center;
  Vector3 f = d * (this->properties->mass * other.GetProperties()->mass / (1+d.GetSquareMag()));
  this->ApplyForce(game, f);
}

void
Mob::ApplyForce(Game &game, const Vector3 &f) {
  velocity = velocity + f * (game.GetDeltaT() / this->properties->mass); 
}

float
Mob::GetMoveModifier() const {
  float mod = 1.0;
  if (sneak) mod *= 0.5;
  if (this->footCell) mod *= this->footCell->GetInfo().speedModifier;
  return mod * (1.0 + GetEffectiveStats().agi * Const::WalkSpeedFactorPerAGI);
}
