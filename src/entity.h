#ifndef BARFOOS_ENTITY_H
#define BARFOOS_ENTITY_H

#include "common.h"
#include "icolor.h"
#include "inventory.h"

class Cell;
class Game;
class Gfx;

struct EntityProperties {
  // rendering
  Sprite sprite;
  bool isBox = false;
  IColor glow;
  
  // movement
  float stepHeight = 0.5f;
  float mass = 1;
  float moveInterval = 0;
  float maxSpeed = 2;

  // gameplay
  int maxHealth = 5;
  Vector3 extents;
  bool nohit = false;
  bool nocollideEntity = false;
  bool nocollideCell = false;
  bool nocollideOwner = false;
  bool noFriction = false;
  bool isSolid = false;
  float gravity = 1.0;
  float eyeOffset = 0.0;
  
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

  virtual void Start(Game &game, size_t id);
  virtual void Update(Game &game);
  virtual void Think(Game &game);

  virtual void Draw(Gfx &gfx) const;
  virtual void DrawBoundingBox(Gfx &gfx) const;
  
  virtual void AddHealth(Game &game, const HealthInfo &info); 
  virtual void Die(Game &game, const HealthInfo &info);
  
  virtual void OnCollide(Game &game, Entity &other) { (void)game; (void)other; }
  virtual void OnCollide(Game &game, Cell &cell, Side side) { (void)game; (void)cell; (void)side; }
  virtual void OnUse(Game &game, Entity &other) { (void)game; (void)other; }
  
  // management
  size_t GetId() const { return id; }
  bool IsRemovable() const { return removable; }
  const EntityProperties *GetProperties() const { return properties; }

  size_t GetOwner() const { return ownerId; }
  void SetOwner(const Entity &owner) { ownerId = owner.id; }
  
  // gameplay
  bool IsSolid() const { return properties->isSolid; }
  Inventory &GetInventory() { return inventory; }
  
  void SetSpawnPos(const Vector3 &p)    { this->spawnPos = p; }

  void SetPosition(const Vector3 &pos)  { smoothPosition = aabb.center = pos; }
  void SetPosition(const IVector3 &pos) { SetPosition(Vector3(pos) + Vector3(0.5,0.5,0.5)); }
  const Vector3 &GetPosition() const    { return aabb.center; }
  
  void SetAngles(const Vector3 &angles) { this->angles = angles; }
  const Vector3 &GetAngles() const      { return angles; }
  Vector3 GetForward() const            { return GetAngles().EulerToVector(); }
  
  const AABB &GetAABB() const           { return aabb; }
  
  // rendering
  virtual IColor GetLight() const       { return properties->glow + inventory.GetLight(); }
  
protected:

  // management
  size_t id, ownerId;
  bool removable;
  const EntityProperties *properties;
  
  // gameplay
  Vector3 smoothPosition;
  Vector3 lastPos;
  Vector3 spawnPos;
  Vector3 angles;
  
  AABB aabb;
  
  int health;
  
  Cell *lastCell;
  IVector3 cellPos;
  Inventory inventory;
  
  // rendering
  Sprite sprite;
  bool drawAABB;
  IColor cellLight;
  
};

#endif
