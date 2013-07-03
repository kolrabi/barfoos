#ifndef BARFOOS_ENTITY_H
#define BARFOOS_ENTITY_H

#include "common.h"

class Cell;
class Game;
class Item;
class Gfx;

struct IColor;

struct EntityProperties {
  // rendering
  Sprite sprite;
  
  // movement
  float stepHeight = 0.5f;
  float mass = 1;
  float moveInterval = 0;
  float maxSpeed = 2;

  // gameplay
  int maxHealth = 5;
  Vector3 extents;
  bool nohit = false;
  
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
  
  static float ThinkInterval;

  const AABB &GetAABB() const { return aabb; }
  void SetPosition(const Vector3 &pos) { aabb.center = pos; }
  void SetPosition(const IVector3 &pos) { 
    smoothPosition = aabb.center = Vector3(pos) + Vector3(0.5,0.5,0.5); 
  }
  
  virtual void Start();
  virtual void Update();
  virtual void Think();

  virtual void Draw(Gfx &gfx) const;
  virtual void DrawBoundingBox(Gfx &gfx) const;
  
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
  const EntityProperties *GetProperties() const { return properties; }
  
protected:

  bool removable;
  
  AABB aabb;
  Vector3 smoothPosition;
  Vector3 lastPos;
  
  Sprite sprite;
    
  std::vector<std::shared_ptr<Item>> inventory;
  const EntityProperties *properties;
  
  Cell *lastCell;
  IVector3 cellPos;
  
  int health;
};

#endif
