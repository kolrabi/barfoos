#include "mob.h"
#include "world.h"
#include "cell.h"
#include "util.h"

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
Mob::Update(float t) {

  Entity::Update(t);
  
  if (!this->world) return;
  
  if (this->properties->moveInterval != 0) {
    if (t > nextMoveT) {
      nextMoveT += this->properties->moveInterval;
      moveTarget = aabb.center + (Vector3::Rand()-Vector3(0.5,0.5,0.5)) * 4.0;
      validMoveTarget = true;
    }
    
    Vector3 tmove = (moveTarget - aabb.center).Horiz();
    if (tmove.GetMag() < 0.1) validMoveTarget = false;
    if (validMoveTarget) move = tmove.Normalize() * this->properties->maxSpeed;
  }
 
  float speed = move.GetMag();
  if (speed > this->properties->maxSpeed) move = move * (this->properties->maxSpeed/speed);
  velocity.x = std::abs(move.x) > std::abs(velocity.x) ? move.x : velocity.x;
  velocity.z = std::abs(move.z) > std::abs(velocity.z) ? move.z : velocity.z;

  if (noclip) {
    if (wantJump) {
      velocity.y = 3;
      wantJump = false;
    } else if (sneak) {
      velocity.y = -3;
    }
  } else if (inWater) { 
    if (wantJump) {
      velocity.y += 9.81*deltaT*3 * (underWater?2:1);
      wantJump = false;
    } else {
      velocity.y -= 9.81*deltaT*3;
    }
    velocity.x = velocity.x / (1+deltaT*10);
    velocity.y = velocity.y / (1+deltaT*10);
    velocity.z = velocity.z / (1+deltaT*10);
  } else {
    velocity.y -= 9.81*deltaT*3;
    if (wantJump && onGround) {
      if (t - lastJumpT > 0.5) {
        velocity.y = 8;
        lastJumpT = t;
      }
    }
    wantJump = false;
  }
  velocity.x = velocity.x / (1+deltaT*10);
  velocity.z = velocity.z / (1+deltaT*10);

  // move
  uint8_t axis, axis2;
  bool movingDown = velocity.y <= 0;

  if (noclip) {
    aabb.center = aabb.center + velocity*deltaT;
  } else if (movingDown) {
    // when moving down and pushing use step height
    Vector3 step(0, move.GetMag()!=0 ? this->properties->stepHeight : 0, 0);
    Vector3 org = aabb.center + velocity.Horiz()*deltaT;
    
    aabb.center = this->world->MoveAABB(aabb, aabb.center + step, axis2);
    aabb.center = this->world->MoveAABB(aabb, aabb.center + velocity.Horiz()*deltaT, axis);
    aabb.center = this->world->MoveAABB(aabb, aabb.center - step*1.25 + velocity.Vert()*deltaT, axis2);
    org.y = aabb.center.y;
    axis |= axis2;
    aabb.center = this->world->MoveAABB(aabb, org, axis2);
    axis |= axis2;
    if (!axis2 & Axis::Y) aabb.center = aabb.center + step*0.25;
  } else if (!movingDown) {
    onGround = false;
    aabb.center = this->world->MoveAABB(aabb, aabb.center + velocity*deltaT, axis);
  }

  // jump out of water
  if (axis & Axis::Horizontal && (inWater || validMoveTarget) && move.GetMag() != 0) {
    wantJump = true;
  }

  IVector3 footPos(aabb.center.x, aabb.center.y - aabb.extents.y + this->properties->stepHeight, aabb.center.z);
  footCell = &this->world->GetCell(footPos);

  if (onGround) {
    groundCell = &this->world->GetCell(footPos[Side::Down]);
  } else {
    groundCell = nullptr;
  }
  
  IVector3 headPos(aabb.center.x, aabb.center.y + aabb.extents.y, aabb.center.z);
  headCell = &this->world->GetCell(headPos);

  underWater = headCell->GetInfo().flags & CellFlags::Liquid;
  if (!noclip) this->SetInLiquid(footCell->GetInfo().flags & CellFlags::Liquid);
  
  if (axis & Axis::Y) {
    if (velocity.y < -15) {
      AddHealth((velocity.y+15)/5);
      std::cerr << velocity.y << std::endl;
    }
    velocity.y = 0;
    onGround |= movingDown;
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
Mob::Die() {
  health = this->properties->maxHealth;
  SetPosition(spawnPos);
}

void
Mob::OnCollide(const std::shared_ptr<Entity> &other) {
  Vector3 d = this->GetAABB().center - other->GetAABB().center;
  Vector3 f = d * (100 / (1+d.GetSquareMag()));
  this->ApplyForce(f);
}
