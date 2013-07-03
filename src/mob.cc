#include "mob.h"
#include "world.h"
#include "cell.h"
#include "util.h"
#include "game.h"

#include <cmath>

Mob::Mob(const std::string &propertyName) : Entity(propertyName) {
  this->sprite.t = Game::Instance->GetWorld()->GetRandom().Float01();
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
Mob::Update() {
  Entity::Update();

  float deltaT = Game::Instance->GetDeltaT();
  
  velocity.x = std::abs(move.x) > std::abs(velocity.x) ? move.x : velocity.x;
  velocity.z = std::abs(move.z) > std::abs(velocity.z) ? move.z : velocity.z;
  
  float gravity = 3*9.81*deltaT;
  float friction = 1.0 / (1.0+deltaT * 10);

  if (noclip) {
    if (wantJump) {
      velocity.y = 3;
      wantJump = false;
    } else if (sneak) {
      velocity.y = -3;
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
      if (Game::Instance->GetTime() - lastJumpT > 0.5) {
        velocity.y = 8;
        lastJumpT = Game::Instance->GetTime();
      }
    }
    wantJump = false;
  }
  velocity.x = velocity.x * friction;
  velocity.z = velocity.z * friction;

  // move
  std::shared_ptr<World> world = Game::Instance->GetWorld();
  uint8_t axis, axis2;
  bool movingDown = velocity.y <= 0;

  if (noclip) {
    aabb.center = aabb.center + velocity * deltaT;
  } else if (movingDown) {
    // when moving down and pushing use step height
    Vector3 step(0, move.GetMag()!=0 ? this->properties->stepHeight : 0, 0);
    Vector3 org = aabb.center + velocity.Horiz() * deltaT;
    
    aabb.center = world->MoveAABB(aabb, aabb.center + step, axis2);
    aabb.center = world->MoveAABB(aabb, aabb.center + velocity.Horiz()*deltaT, axis);
    aabb.center = world->MoveAABB(aabb, aabb.center - step*1.25 + velocity.Vert()*deltaT, axis2);
    org.y = aabb.center.y;
    axis |= axis2;
    aabb.center = world->MoveAABB(aabb, org, axis2);
    axis |= axis2;
    if (!axis2 & Axis::Y) aabb.center = aabb.center + step*0.25;
  } else if (!movingDown) {
    onGround = false;
    aabb.center = world->MoveAABB(aabb, aabb.center + velocity*deltaT, axis);
  }

  // jump out of water
  if (axis & Axis::Horizontal && (inWater || validMoveTarget) && move.GetMag() != 0) {
    wantJump = true;
  }

  // fall damage  
  if (axis & Axis::Y) {
    if (velocity.y < -15) {
      AddHealth((velocity.y+15)/5);
    }
    velocity.y = 0;
    onGround |= movingDown;
  }
}

void
Mob::Think() {  
  Entity::Think();
  
  // clip move speed
  float speed = move.GetMag();
  if (speed > this->properties->maxSpeed) move = move * (this->properties->maxSpeed/speed);

  // walk around a bit
  if (this->properties->moveInterval != 0) {
    if (Game::Instance->GetTime() > nextMoveT) {
      nextMoveT += this->properties->moveInterval;
      moveTarget = aabb.center + (Vector3::Rand()-Vector3(0.5,0.5,0.5)) * 4.0;
      validMoveTarget = true;
    }
    
    Vector3 tmove = (moveTarget - aabb.center).Horiz();
    if (tmove.GetMag() < 0.1) validMoveTarget = false;
    if (validMoveTarget) move = tmove.Normalize() * this->properties->maxSpeed;
  }
 
  std::shared_ptr<World> world = Game::Instance->GetWorld();
  IVector3 footPos(aabb.center.x, aabb.center.y - aabb.extents.y + this->properties->stepHeight, aabb.center.z);
  footCell = &world->GetCell(footPos);

  if (onGround) {
    groundCell = &world->GetCell(footPos[Side::Down]);
  } else {
    groundCell = nullptr;
  }
  
  IVector3 headPos(aabb.center.x, aabb.center.y + aabb.extents.y, aabb.center.z);
  headCell = &world->GetCell(headPos);

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
Mob::Die() {
  health = this->properties->maxHealth;
  SetPosition(spawnPos);
}

void
Mob::OnCollide(Entity &other) {
  Vector3 d = this->GetAABB().center - other.GetAABB().center;
  Vector3 f = d * (this->properties->mass * other.GetProperties()->mass / (1+d.GetSquareMag()));
  this->ApplyForce(f);
}

void
Mob::ApplyForce(const Vector3 &f) {
  velocity = velocity + f * (Game::Instance->GetDeltaT() / this->properties->mass); 
}
