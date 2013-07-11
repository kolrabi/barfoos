#include "mob.h"
#include "world.h"
#include "cell.h"
#include "util.h"
#include "game.h"

#include <cmath>

Mob::Mob(const std::string &propertyName) : Entity(propertyName) {
  this->onGround = false;
  this->wantJump = false;
  this->lastJumpT = 0;
  
  this->inWater  = false;
  this->underWater  = false;

  this->headCell = this->groundCell = this->footCell = nullptr;

  this->sneak = false;
  this->noclip = false;
  
  this->nextMoveT = 0;
  this->validMoveTarget = false;
}

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

  float gravity = 3 * 9.81 * deltaT;
  float footFriction = 1.0 / (this->footCell ? this->footCell->GetInfo().speedModifier : 1.0);
  float groundFriction = this->groundCell ? this->groundCell->GetInfo().friction : 0.1;
  float friction = 1.0 / (1.0+deltaT * 10 * footFriction * groundFriction);

  float tvx = std::abs(move.x) > std::abs(velocity.x) ? move.x : velocity.x;
  float tvz = std::abs(move.z) > std::abs(velocity.z) ? move.z : velocity.z;
  
  velocity.x += (tvx-velocity.x) * groundFriction * deltaT * 10;
  velocity.z += (tvz-velocity.z) * groundFriction * deltaT * 10;
  
  if (noclip) {
    if (wantJump) {
      velocity.y = 3;
      wantJump = false;
    } else if (sneak) {
      velocity.y = -3;
      sneak = false;
    } else {
      velocity.y = 0;
    }
  } else if (inWater) { 
    if (wantJump) {
      velocity.y += gravity * (underWater?2:1);
      wantJump = false;
    } else {
      velocity.y -= gravity;
    }
    velocity.y = velocity.y * friction;
  } else {
    velocity.y -= gravity;
    if (wantJump && onGround) {
      if (game.GetTime() - lastJumpT > 0.5) {
        velocity.y = 8;
        lastJumpT = game.GetTime();
      }
    }
    wantJump = false;
  }
  velocity.x = velocity.x * friction;
  velocity.z = velocity.z * friction;

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
    bool downStep = false;
    
    aabb.center = world.MoveAABB(aabb, aabb.center + step, axis2);
    aabb.center = world.MoveAABB(aabb, aabb.center + velocity.Horiz()*deltaT, axis, &cell, &side);
    aabb.center = world.MoveAABB(aabb, aabb.center - step*1.25 + velocity.Vert()*deltaT, axis2);
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
  
  if (cell) {
    std::cerr << cell->GetPosition() << " " << side << std::endl;
  }

  // jump out of water
  if (axis & Axis::Horizontal && (inWater || validMoveTarget) && move.GetMag() != 0 && !noclip && !sneak) {
    wantJump = true;
  }

  // fall damage  
  if (axis & Axis::Y) {
    if (velocity.y < -15) {
      AddHealth(game, HealthInfo( (velocity.y+15)/5, HealthType::Falling, this->id));
    }
    velocity.y = 0;
    onGround |= movingDown;
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
 
  World &world = game.GetWorld();
  IVector3 footPos(aabb.center.x, aabb.center.y - aabb.extents.y + this->properties->stepHeight, aabb.center.z);
  footCell = &world.GetCell(footPos);

  if (onGround) {
    groundCell = &world.GetCell(footPos[Side::Down]);
  } else {
    groundCell = nullptr;
  }
  
  IVector3 headPos(aabb.center.x, aabb.center.y + aabb.extents.y, aabb.center.z);
  headCell = &world.GetCell(headPos);

  underWater = headCell->GetInfo().flags & CellFlags::Liquid;
  if (!noclip) this->SetInLiquid(footCell->GetInfo().flags & CellFlags::Liquid);
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
  // Entity::Die(game);
  (void)info;
  (void)game;
  
  // just respawn
  health = this->properties->maxHealth;
  SetPosition(spawnPos);
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
  return mod;
}