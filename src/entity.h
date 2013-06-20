#ifndef BARFOOS_ENTITY_H
#define BARFOOS_ENTITY_H

#include "common.h"

class Cell;
class World;
class Item;

struct EntityProperties {
  // rendering
  unsigned int texture = 0;
  size_t frames = 1;
  std::vector<Animation> anims;
  float w = 1.0, h = 1.0;
  float originX = 0.5, originY = 0.5;
  
  // movement
  float stepHeight = 0.5f;
  float mass = 1;
  float moveInterval = 0;
  float maxSpeed = 2;

  // gameplay
  int maxHealth = 5;
  Vector3 extents;
  
  std::string cellEnter, cellLeave;
  
  EntityProperties();
  EntityProperties(FILE *f);
};

void LoadEntities();
const EntityProperties *getEntity(const std::string &name);

class Entity {
public:
  Entity(const std::string &visualName);
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
  virtual void Die();
  virtual void OnCollide(const std::shared_ptr<Entity> &other) { (void)other; }
  
  bool IsRemovable() const { return removable; }

protected:

  World *world;
  bool removable;
  
  AABB aabb;
  Vector3 smoothPosition;
  float lastT, deltaT;
  float frame;
  size_t animation;
    
  std::vector<std::shared_ptr<Item>> inventory;
  const EntityProperties *properties;
  
  Cell *lastCell;
  
  IColor light;

  int health;
};

#endif
