#ifndef BARFOOS_ENTITY_H
#define BARFOOS_ENTITY_H

#include "common.h"

class Cell;
class World;
class Item;

class Entity {
public:
  Entity();
  virtual ~Entity();

  void SetWorld(World *world) { this->world = world; }
  World *GetWorld() { return this->world; }
  
  const AABB &GetAABB() const { return aabb; }
  void SetPosition(const Vector3 &pos) { aabb.center = pos; }
  void SetPosition(const IVector3 &pos) { 
    smoothPosition = aabb.center = Vector3(pos) + Vector3(0.5,0.5,0.5); 
  }
  
  virtual void Update(float t);
  virtual void Draw();
  
  virtual void DrawBoundingBox();
  virtual void AddHealth(int points); 
  virtual void Die() {}

protected:

  World *world;
  
  AABB aabb;
  Vector3 smoothPosition;
  float lastT, deltaT;
  
  size_t frames;
  float frame;
  size_t animation;
  std::vector<Animation> anims;
  
  std::vector<std::shared_ptr<Item>> inventory;
  
  unsigned int texture, aboveTexture, belowTexture;
  
  IColor light;

  int health;
  int maxHealth;
};

#endif
