#ifndef BARFOOS_ENTITY_H
#define BARFOOS_ENTITY_H

#include "common.h"
#include "icolor.h"
#include "inventory.h"
#include "aabb.h"
#include "sprite.h"
#include "stats.h"
#include "util.h"
#include "ivector3.h"
#include "vector3.h"
#include "smooth.h"

#include "properties.h"

struct EntityDrawBox {
  const Texture *texture = nullptr;
  AABB aabb = AABB();
};

struct EntityProperties : public Properties {

  // rendering
  Sprite  sprite            = Sprite();
  bool    isBox             = false;  //< Render entity as a box instead of a sprite.
  IColor  glow              = IColor(0,0,0);
  size_t  flinchAnim        = ~0UL;   //< Animation to play when hurt.
  size_t  dyingAnim         = ~0UL;   //< Animation to play on death.
  std::vector<EntityDrawBox> drawBoxes = std::vector<EntityDrawBox>(0);
  
  // movement
  float   stepHeight        = 0.5;    //< Allow entity to climb stairs.
  float   mass              = 1.0;    //< Mass of entity.
  float   moveInterval      = 0.0;    //< Entity will search a new target location after this amount of time.
  float   maxSpeed          = 2.0;    //< Maximum (horizontal) move speed.
  float   gravity           = 1.0;    //< Amount of effect of gravity on entity.

  // gameplay
  Vector3 extents           = Vector3(0.5,0.5,0.5); //< Size of the entity.
  float   eyeOffset         = 0.0;    //< Offset from the center.
  float   thinkInterval     = 0.0;    //< Time interval beetween calls to Entity::Think().
  float   maxHealth         = 5;      //< Health of entity after spawn.
  bool    nohit             = false;  //< Entity has no hitbox/selection box.
  bool    nocollideEntity   = false;  //< Entity does not collide with other entities.
  bool    nocollideCell     = false;  //< Don't call OnCollideCell.
  bool    nocollideOwner    = false;  //< Entity does not collide with owner entity.
  bool    noFriction        = false;  //< Entity is unaffected by friction.
  bool    isSolid           = false;  //< Entity is solid in collision detection.
  bool    respawn           = false;  //< Entity will automatically respawn on death.
  float   exp               = 0.0;

  // stats
  int     str               = 0;
  int     dex               = 0;
  int     agi               = 0;
  int     def               = 0;
  
  /** Inventory items and their absolute probability. */
  std::vector<std::pair<std::string, float>> items = std::vector<std::pair<std::string, float>>(0);
  
  /** When entering a cell, replace it with a cell of this type. */
  std::string cellEnter     = "";

  /** When leaving a cell, replace it with a cell of this type. */
  std::string cellLeave     = "";
  
  /** Display name of entity. */
  std::string name          = "";

  std::string group         = "";
  
  std::unordered_map<std::string, std::string> onUseItemReplace;
  
  virtual void ParseProperty(const std::string &name) override;
};

void LoadEntities();
const EntityProperties *getEntity(const std::string &name);
std::vector<std::string> GetEntitiesInGroup(const std::string &group);

class Entity {
public:
  Entity(const std::string &visualName);
  Entity(const Entity &that) = delete;
  virtual ~Entity();
  
  Entity &operator=(const Entity &that) = delete;
  
  virtual void Start(Game &game, size_t id);
  virtual void Update(Game &game);
  virtual void Think(Game &game);

  virtual void Draw(Gfx &gfx) const;
  virtual void DrawBoundingBox(Gfx &gfx) const;
  
  virtual void AddHealth(Game &game, const HealthInfo &info); 
  virtual void Die(Game &game, const HealthInfo &info);
  
  virtual void OnCollide(Game &game, Entity &other)           { (void)game; (void)other; }
  virtual void OnCollide(Game &game, Cell &cell, Side side)   { (void)game; (void)cell; (void)side; }
  virtual void OnUse(Game &game, Entity &other)               { (void)game; (void)other; }
  
  virtual void OnHealthDealt(Game &game, Entity &other, const HealthInfo &info);
  virtual void OnLevelUp(Game &game)                          { (void)game; }
  virtual void OnEquip(Game &, const Item &, InventorySlot, bool)      {}
  
  // management
  size_t                    GetId()                           const { return id; }
  bool                      IsRemovable()                     const { return removable; }
  const EntityProperties *  GetProperties()                   const { return properties; }
  virtual std::string       GetName()                         const { return properties->name; }

  size_t                    GetOwner()                        const { return ownerId; }
  void                      SetOwner(const Entity &owner)           { this->ownerId = owner.id; }
  
  // gameplay
  
  bool                      IsSolid()                         const { return properties->isSolid; }
  bool                      IsDead()                          const { return this->health <= 0 && this->properties->maxHealth != 0;}
  Inventory &               GetInventory()                          { return this->inventory; }
  
  void                      SetSpawnPos(const Vector3  &p)          { this->spawnPos = p; }

  void                      SetPosition(const Vector3  &pos)        { this->smoothPosition.SnapTo( this->aabb.center = pos ); }
  void                      SetPosition(const IVector3 &pos)        { this->SetPosition(Vector3(pos) + Vector3(0.5,0.5,0.5)); }
  const Vector3 &           GetPosition()                     const { return this->aabb.center; }
  const Vector3 &           GetSmoothPosition()               const { return this->smoothPosition; }
  const Vector3             GetEyePosition()                  const { return this->GetPosition() + Vector3(0,this->properties->eyeOffset,0); }
  const Vector3             GetSmoothEyePosition()            const { return this->GetSmoothPosition() + Vector3(0,this->properties->eyeOffset,0); }
  
  void                      SetAngles(const Vector3 &angles)        { this->angles = angles; }
  const Vector3 &           GetAngles()                       const { return this->angles; }
  Vector3                   GetForward()                      const { return this->GetAngles().EulerToVector(); }
  Vector3                   GetRight()                        const { return (this->GetAngles() + Vector3(Const::pi_2, 0, 0)).EulerToVector(); }
  
  const AABB &              GetAABB()                         const { return this->aabb; }
  Stats                     GetEffectiveStats()               const;
  Stats &                   GetBaseStats()                          { return this->baseStats; }
  float                     GetHealth()                       const { return this->health; }
  
  void                      AddBuff(Game &game, const std::string &name);
  
  // rendering
  virtual IColor            GetLight()                        const { return this->properties->glow + inventory.GetLight(); }
  
protected:

  // management
  size_t id, ownerId;
  bool removable;
  const EntityProperties *properties;
  float nextThinkT, startT;
  
  // gameplay
  Smooth<Vector3> smoothPosition = Smooth<Vector3>(30.0);
  Vector3 lastPos;
  Vector3 spawnPos;
  Vector3 angles;
  
  Stats baseStats;
  std::vector<Buff> activeBuffs;
  
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
