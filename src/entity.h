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

enum class SpawnClass : char {
  EntityClass = 'E', 
  MobClass    = 'M', 
  ItemEntityClass = 'I',
  PlayerClass = 'P',
  ParticleClass = 'A',
  ProjectileClass = 'R'
};

struct EntityDrawBox {
  const Texture *texture = nullptr;
  AABB aabb = AABB();
};

struct ParticleEmitter {
  AABB aabb;
  std::string name;
  Vector3 velocity;
  float rate;
  float state;
};

struct EntityProperties : public Properties {

  // rendering
  Sprite  sprite            = Sprite();
  bool    isBox             = false;  //< Render entity as a box instead of a sprite.
  IColor  glow              = IColor(0,0,0);
  size_t  flinchAnim        = ~0UL;   //< Animation to play when hurt.
  size_t  dyingAnim         = ~0UL;   //< Animation to play on death.
  size_t  attackAnim        = ~0UL;   //< Animation to play on death.
  std::vector<EntityDrawBox> drawBoxes = std::vector<EntityDrawBox>(0);
  std::vector<ParticleEmitter> emitters = std::vector<ParticleEmitter>(0);
  bool    createBubbles     = false;
  
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
  float   lifetime          = 0.0;
  bool    nohit             = false;  //< Entity has no hitbox/selection box.
  bool    nocollideEntity   = false;  //< Entity does not collide with other entities.
  bool    nocollideCell     = false;  //< Don't call OnCollideCell.
  bool    nocollideOwner    = false;  //< Entity does not collide with owner entity.
  bool    noFriction        = false;  //< Entity is unaffected by friction.
  bool    isSolid           = false;  //< Entity is solid in collision detection.
  bool    respawn           = false;  //< Entity will automatically respawn on death.
  bool    aggressive        = false;
  bool    onCollideUseCell  = false;
  bool    swim              = false;
  bool    flipLeft          = false;
  float   jumpSpeed         = 8.0;
  float   attackInterval    = 0.0;
  float   aggroRangeNear    = 0.0;
  float   aggroRangeFar     = 0.0;
  float   meleeAttackRange  = 0.0;
  std::string attackItem    = "";
  float   attackForwardStep = 0.0;
  float   attackJump        = 0.0;
  float   exp               = 0.0;

  size_t  onDieExplodeRadius = 0;
  float   onDieExplodeStrength = 0.0;
  float   onDieExplodeDamage = 0.0;
  Element onDieExplodeElement = Element::Physical;

  size_t  onDieParticles    = 0;
  float   onDieParticleSpeed = 0.0;
  std::string onDieParticleType = "";

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
  std::string displayName   = "";

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
  Entity(const std::string &type);
  Entity(const std::string &type, Deserializer &deser);
  Entity(const Entity &that) = delete;
  virtual ~Entity();
  
  Entity &operator=(const Entity &that) = delete;
  
  virtual void Start(RunningState &state, size_t id);
  virtual void Continue(RunningState &state, size_t id);
  virtual void Update(RunningState &state);
  virtual void Think(RunningState &state);

  virtual void Draw(Gfx &gfx) const;
  virtual void DrawBoundingBox(Gfx &gfx) const;
  
  virtual void AddHealth(RunningState &state, const HealthInfo &info); 
  virtual void Die(RunningState &state, const HealthInfo &info);
  
  virtual void OnCollide(RunningState &, Entity &)            {}
  virtual void OnCollide(RunningState &, Cell &, Side)        {}
  virtual void OnUse(RunningState &, Entity &)                {}
  
  virtual void OnHealthDealt(RunningState &, Entity &other, const HealthInfo &info);
  virtual void OnLevelUp(RunningState &)                          {}
  virtual void OnEquip(RunningState &, const Item &, InventorySlot, bool)      {}
  virtual void OnBuffAdded(RunningState &, const EffectProperties &)      {}
  
  // management
  size_t                    GetId()                           const { return id; }
  bool                      IsRemovable()                     const { return removable; }
  const EntityProperties *  GetProperties()                   const { return properties; }
  virtual std::string       GetName()                         const { return properties->displayName; }

  size_t                    GetOwner()                        const { return ownerId; }
  void                      SetOwner(const Entity &owner)           { this->ownerId = owner.id; }
  
  // gameplay
  
  bool                      IsSolid()                         const { return properties->isSolid; }
  bool                      IsDead()                          const { return this->isDead; }
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
  
  bool                      CanSee(RunningState &state, const Vector3 &pos);
  
  void                      AddBuff(RunningState &state, const std::string &name);
  
  // rendering
  virtual IColor            GetLight()                        const { return this->properties->glow + inventory.GetLight(); }

  virtual void              Serialize(Serializer &ser)        const;
  
protected:

  // management
  size_t id, ownerId;
  bool removable;
  const EntityProperties *properties;
  float nextThinkT, startT;

  std::unordered_map<std::string, Regular> regulars;
  
  // gameplay
  float dieT;
  bool isDead;
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
  std::vector<ParticleEmitter> emitters = std::vector<ParticleEmitter>(0);
  
  virtual SpawnClass GetSpawnClass() const { return SpawnClass::EntityClass; }
  
  friend Serializer &operator << (Serializer &ser, const Entity *entity);
  friend Deserializer &operator >> (Deserializer &deser, Entity *&entity);
};

#endif
