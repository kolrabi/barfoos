#ifndef BARFOOS_ENTITY_H
#define BARFOOS_ENTITY_H

#include "common.h"
#include "icolor.h"

class Cell;
class Game;
class Item;
class Gfx;

struct EntityProperties {
  // rendering
  Sprite sprite;
  bool isBox = false;
  
  // movement
  float stepHeight = 0.5f;
  float mass = 1;
  float moveInterval = 0;
  float maxSpeed = 2;

  // gameplay
  int maxHealth = 5;
  Vector3 extents;
  bool nohit = false;
  bool nocollide = false;
  bool isSolid = false;
  
  std::vector<std::pair<std::string, float>> items;
  
  std::string cellEnter, cellLeave;
  
  EntityProperties();
  EntityProperties(FILE *f);
};

enum class HealthType {
  Unspecified = 0,
  Heal,
  Fire,
  Lava,
  Falling,
  Explosion,
  Melee,
  Ranged,
  Vampiric
};

struct HealthInfo {
  int amount = 0;
  HealthType type = HealthType::Unspecified;
  size_t dealerId = ~0UL;
  
  HealthInfo() {}
  HealthInfo(int amount, HealthType type = HealthType::Unspecified, size_t dealerId = ~0UL) 
    : amount(amount), type(type), dealerId(dealerId) { }
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
  
  virtual void Start(Game &game, size_t id);
  virtual void Update(Game &game);
  virtual void Think(Game &game);

  virtual void Draw(Gfx &gfx) const;
  virtual void DrawBoundingBox(Gfx &gfx) const;
  
  virtual void AddHealth(Game &game, const HealthInfo &info); 
  virtual void Die(Game &game, const HealthInfo &info);
  virtual void OnCollide(Game &game, Entity &other) { (void)game; (void)other; }
  virtual void OnUse(Game &game, Entity &other) { (void)game; (void)other; }
  
  bool IsRemovable() const { return removable; }
  
  bool AddToInventory(const std::shared_ptr<Item> &item);
  bool AddToInventory(const std::shared_ptr<Item> &item, InventorySlot slot);

  size_t GetInventorySize() const { return inventory.size(); }
  std::shared_ptr<Item> GetInventory(InventorySlot slot) { 
    return inventory[(size_t)slot]; 
  }

  void Equip(const std::shared_ptr<Item> &item, InventorySlot slot);
  const EntityProperties *GetProperties() const { return properties; }
  
  size_t GetId() const { return id; }
  bool IsSolid() const { return properties->isSolid; }
  
protected:

  size_t id;
  bool removable;
  
  AABB aabb;
  Vector3 smoothPosition;
  Vector3 lastPos;
  
  Sprite sprite;
  bool drawAABB;
    
  std::vector<std::shared_ptr<Item>> inventory;
  const EntityProperties *properties;
  
  Cell *lastCell;
  IVector3 cellPos;
  IColor cellLight;
  
  int health;
};

#endif
