#ifndef BARFOOS_ENTITY_H
#define BARFOOS_ENTITY_H

#include "common.h"

class Cell;
class Game;
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
  
  std::map<std::string, float> items;
  
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

  const AABB &GetAABB() const { return aabb; }
  void SetPosition(const Vector3 &pos) { aabb.center = pos; }
  void SetPosition(const IVector3 &pos) { 
    smoothPosition = aabb.center = Vector3(pos) + Vector3(0.5,0.5,0.5); 
  }
  
  virtual void Start();
  virtual void Update();
  virtual void Draw() const;
  virtual void DrawBoundingBox() const;
  
  virtual void AddHealth(int points); 
  virtual void Die();
  virtual void OnCollide(Entity &other) { (void)other; }
  virtual void OnUse(Entity &other) { (void)other; }
  
  bool IsRemovable() const { return removable; }
  bool AddToInventory(const std::shared_ptr<Item> &item);
  bool AddToInventory(const std::shared_ptr<Item> &item, InventorySlot slot);

  size_t GetInventorySize() const { return inventory.size(); }
  std::shared_ptr<Item> GetInventory(InventorySlot slot) { 
    return inventory[(size_t)slot]; 
  }

  void Equip(const std::shared_ptr<Item> &item, InventorySlot slot);
  
protected:

  bool removable;
  
  AABB aabb;
  Vector3 smoothPosition;
  
  float frame;
  size_t animation;
    
  std::vector<std::shared_ptr<Item>> inventory;
  const EntityProperties *properties;
  
  Cell *lastCell;
  
  IColor light;

  int health;
};

#endif
