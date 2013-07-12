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
  Sprite  sprite;
  bool    isBox = false;
  IColor  glow;
  
  // movement
  float   stepHeight        = 0.5;
  float   mass              = 1.0;
  float   moveInterval      = 0.0;
  float   maxSpeed          = 2.0;
  float   gravity           = 1.0;

  // gameplay
  Vector3 extents;
  float   eyeOffset         = 0.0;
  float   thinkInterval     = 0.0;
  float   maxHealth         = 5;
  bool    nohit             = false;
  bool    nocollideEntity   = false;
  bool    nocollideCell     = false;
  bool    nocollideOwner    = false;
  bool    noFriction        = false;
  bool    isSolid           = false;
  bool    respawn           = false;
  
  std::vector<std::pair<std::string, float>> items;
  
  std::string cellEnter, cellLeave;
  
  std::string name;
  
  EntityProperties();
  EntityProperties(FILE *f);
};

enum class HealthType : size_t {
  Unspecified = 0,
  Heal,
  Falling,
  Explosion,
  Melee,
  Arrow,
  Vampiric,
  
  Fire,
  Lava
};

static inline bool IsContinuous(HealthType t) { return t == HealthType::Fire || t == HealthType::Lava; }

struct HealthInfo {
  float amount = 0;
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
  
  virtual void Start(Game &game, size_t id);
  virtual void Update(Game &game);
  virtual void Think(Game &game);

  virtual void Draw(Gfx &gfx) const;
  virtual void DrawBoundingBox(Gfx &gfx) const;
  
  virtual void AddHealth(Game &game, const HealthInfo &info); 
  virtual void Die(Game &game, const HealthInfo &info);
  
  virtual void OnCollide(Game &game, Entity &other)          { (void)game; (void)other; }
  virtual void OnCollide(Game &game, Cell &cell, Side side)  { (void)game; (void)cell; (void)side; }
  virtual void OnUse(Game &game, Entity &other)              { (void)game; (void)other; }
  
  // management
  size_t                    GetId()                           const { return id; }
  bool                      IsRemovable()                     const { return removable; }
  const EntityProperties *  GetProperties()                   const { return properties; }
  virtual std::string       GetName()                         const { return properties->name; }

  size_t                    GetOwner()                        const { return ownerId; }
  void                      SetOwner(const Entity &owner)           { ownerId = owner.id; }
  
  // gameplay
  
  bool                      IsSolid()                         const { return properties->isSolid; }
  Inventory &               GetInventory()                          { return inventory; }
  
  void                      SetSpawnPos(const Vector3  &p)          { this->spawnPos = p; }

  void                      SetPosition(const Vector3  &pos)        { smoothPosition = aabb.center = pos; }
  void                      SetPosition(const IVector3 &pos)        { SetPosition(Vector3(pos) + Vector3(0.5,0.5,0.5)); }
  const Vector3 &           GetPosition()                     const { return aabb.center; }
  
  void                      SetAngles(const Vector3 &angles)        { this->angles = angles; }
  const Vector3 &           GetAngles()                       const { return angles; }
  Vector3                   GetForward()                      const { return GetAngles().EulerToVector(); }
  
  const AABB &              GetAABB()                         const { return aabb; }
  
  // rendering
  virtual IColor            GetLight()                        const { return properties->glow + inventory.GetLight(); }
  
protected:

  // management
  size_t id, ownerId;
  bool removable;
  const EntityProperties *properties;
  float nextThinkT;
  
  // gameplay
  Vector3 smoothPosition;
  Vector3 lastPos;
  Vector3 spawnPos;
  Vector3 angles;
  
  AABB aabb;
  
  float health;
  
  Cell *lastCell;
  IVector3 cellPos;
  Inventory inventory;
  
  // rendering
  Sprite sprite;
  bool drawAABB;
  IColor cellLight;
  
};

#endif
