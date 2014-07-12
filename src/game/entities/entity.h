#ifndef BARFOOS_ENTITY_H
#define BARFOOS_ENTITY_H

#include "common.h"
#include "game/gameplay/stats.h"
#include "game/gameplay/trigger.h"
#include "game/items/inventory.h"
#include "gfx/sprite.h"
#include "io/properties.h"
#include "math/aabb.h"
#include "math/ivector3.h"
#include "math/vector3.h"
#include "util/icolor.h"
#include "util/smooth.h"
#include "util/util.h"

#include "entity.pb.h"
#include <iostream>

enum class SpawnClass : char {
  EntityClass = 'E',
  MobClass    = 'M',
  MonsterClass = 'O',
  ItemEntityClass = 'I',
  PlayerClass = 'P',
  ProjectileClass = 'R'
};

struct EntityDrawBox {
  const Texture *texture = nullptr;
  const Texture *emissiveTexture = nullptr;
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

  SpawnClass klass          = SpawnClass::EntityClass;

  // rendering
  Sprite  sprite            = Sprite();
  float   sizeRand          = 0;
  bool    isBox             = false;  //< Render entity as a box instead of a sprite.
  IColor  glow              = IColor(0,0,0);
  ID  flinchAnim            = InvalidID;   //< Animation to play when hurt.
  ID  dyingAnim             = InvalidID;   //< Animation to play on death.
  ID  attackAnim            = InvalidID;   //< Animation to play on death.
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
  int     minLevel          = 0;
  int     maxLevel          = -1;
  float   maxProbability    = 1.0;

  Vector3 extents           = Vector3(0.5,0.5,0.5); //< Size of the entity.
  float   eyeOffset         = 0.0;    //< Offset from the center.
  float   thinkInterval     = 0.0;    //< Time interval beetween calls to Entity::Think().
  float   maxHealth         = 5;      //< Health of entity after spawn.
  float   heal              = 0.0;
  float   lifetime          = 0.0;
  float   lifetimeRand      = 0.0;
  bool    nohit             = false;  //< Entity has no hitbox/selection box.
  bool    noItemUse         = false;  //< Can't use items on entity
  bool    nocollideEntity   = false;  //< Entity does not collide with other entities.
  bool    nocollideCell     = false;  //< Don't call OnCollideCell.
  bool    nocollideOwner    = false;  //< Entity does not collide with owner entity.
  bool    noFriction        = false;  //< Entity is unaffected by friction.
  bool    isSolid           = false;  //< Entity is solid in collision detection.
  bool    respawn           = false;  //< Entity will automatically respawn on death.
  bool    noStep            = false;
  bool    aggressive        = false;
  bool    retaliate         = false;
  bool    onCollideUseCell  = false;
  bool    swim              = false;
  bool    flipLeft          = false;
  bool    openInventory     = false;
  bool    learnEvade        = false;
  float   jumpSpeed         = 8.0;
  float   attackInterval    = 0.0;
  float   aggroRangeNear    = 0.0;
  float   aggroRangeFar     = 0.0;
  float   meleeAttackRange  = 0.0;
  std::string attackItem    = "";
  float   keepDistance      = 0.0;
  float   attackForwardStep = 0.0;
  float   attackJump        = 0.0;
  float   exp               = 0.0;
  float   lockedChance      = 0.0;
  float   impactDamage      = 0.0;
  bool    randomAngle       = false;
  bool    isQuad            = false;
  bool    alignYForward     = false;

  Element element           = Element::Physical;

  uint32_t  onDieExplodeRadius = 0;
  float   onDieExplodeStrength = 0.0;
  float   onDieExplodeDamage = 0.0;
  Element onDieExplodeElement = Element::Physical;
  std::string onDieExplodeAddBuff = "";
  bool    onDieExplodeMagical = false;

  uint32_t  onDieParticles    = 0;
  float   onDieParticleSpeed = 0.0;
  std::string onDieParticleType = "";

  // stats
  int     str               = 0;
  int     agi               = 0;
  int     vit               = 0;
  int     int_              = 0;
  int     dex               = 0;
  int     luk               = 0;

  int     atk               = 0;
  int     def               = 0;
  int     matk              = 0;
  int     mdef              = 0;

  /** Inventory items and their absolute probability. */
  std::vector<std::pair<std::string, float>> items = std::vector<std::pair<std::string, float>>(0);

  /** When entering a cell, replace it with a cell of this type. */
  std::string cellEnter     = "";

  /** When leaving a cell, replace it with a cell of this type. */
  std::string cellLeave     = "";

  /** Display name of entity. */
  std::string displayName   = "";

  std::string name          = "";
  std::vector<std::string> groups;

  /** Replace items that are used on this entity. itemname -> new itemname */
  std::unordered_map<std::string, std::string> onUseItemReplace;

  /** Replace entity if used with an item. itemname -> new entityname */
  std::unordered_map<std::string, std::string> onUseEntityReplace;

  std::unordered_map<std::string, std::string> sounds;

  virtual void ParseProperty(const std::string &name) override;
};

void LoadEntities();
const EntityProperties *getEntity(const std::string &name);
const std::vector<std::string> &GetEntitiesInGroup(const std::string &group);
float GetEntityProbability(const std::string &type, int level);

class Entity : public Triggerable {
public:

  static Entity *Create(const std::string &type);
  //static Entity *Create(const std::string &type, Deserializer &deser);

  Entity(const Entity &that) = delete;
  virtual ~Entity();

  Entity &operator=(const Entity &that) = delete;

  virtual void Start(RunningState &state, ID id);
  virtual void Continue(RunningState &state, ID id);
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
  ID                        GetId()                           const { return this->proto.id(); }
  bool                      IsRemovable()                     const { return removable; }
  const EntityProperties *  GetProperties()                   const { return properties; }
  Inventory &               GetInventory()                          { return this->inventory; }
  virtual std::string       GetName()                         const { return properties->displayName; }

  ID                        GetOwner()                        const { return this->proto.has_owner_id() ? this->proto.owner_id() : InvalidID; }
  void                      SetOwner(const Entity &owner)           { this->proto.set_owner_id(owner.GetId()); }

  // gameplay

  bool                      IsSolid()                         const { return properties->isSolid; }

  void                      SetIsDead(bool dead)                    { this->proto.set_is_dead(dead); }
  bool                      IsDead()                          const { return this->proto.is_dead(); }
  
  void                      SetSpawnPosition(const Vector3  &p)     { this->proto.mutable_spawn_position()->set_x(p.x); 
                                                                      this->proto.mutable_spawn_position()->set_y(p.y); 
                                                                      this->proto.mutable_spawn_position()->set_z(p.z); }
  Vector3                   GetSpawnPosition()                const { return Vector3(this->proto.spawn_position().x(), 
                                                                                     this->proto.spawn_position().y(), 
                                                                                     this->proto.spawn_position().z()); }

  void                      SetPosition(const Vector3  &pos)        { this->smoothPosition.SnapTo( this->aabb.center = pos ); }
  void                      SetPosition(const IVector3 &pos)        { this->SetPosition(Vector3(pos) + Vector3(0.5,0.5,0.5)); }
  const Vector3 &           GetPosition()                     const { return this->aabb.center; }

  void                      SetForward(const Vector3  &p)           { this->proto.mutable_forward()->set_x(p.x); 
                                                                      this->proto.mutable_forward()->set_y(p.y); 
                                                                      this->proto.mutable_forward()->set_z(p.z); }
  Vector3                   GetForward()                      const { return Vector3(this->proto.forward().x(), 
                                                                                     this->proto.forward().y(), 
                                                                                     this->proto.forward().z()); }
  Vector3                   GetRight()                        const { return this->GetForward().Cross(Vector3(0,1,0)); }

  const Vector3 &           GetSmoothPosition()               const { return this->smoothPosition; }
  const Vector3             GetEyePosition()                  const { return this->GetPosition() + Vector3(0,this->properties->eyeOffset,0); }
  const Vector3             GetSmoothEyePosition()            const { return this->GetSmoothPosition() + Vector3(0,this->properties->eyeOffset,0); }

  void                      Teleport(RunningState &state, const Vector3 &target);
  void                      PlaySound(RunningState &state, const std::string &sound);
  void                      PlaySound(const std::string &sound);


  const AABB &              GetAABB()                         const { return this->aabb; }
  Stats                     GetEffectiveStats()               const;
  Stats &                   GetBaseStats()                          { return this->baseStats; }
  float                     GetHealth()                       const { return this->proto.health(); }

  Element                   GetElement()                      const { return this->properties->element; }

  bool                      CanSee(RunningState &state, const Vector3 &pos);

  void                      AddBuff(RunningState &state, const std::string &name);
  void                      Lock(ID id)                             { this->proto.set_locked_id(id); }
  void                      Unlock()                                { this->proto.set_locked_id(0); }
  ID                        GetLockedID()                     const { return this->proto.locked_id(); }
  bool                      CanOpenInventory()                const { return this->properties->openInventory && this->GetLockedID() == 0; }
  uint32_t                  GetGold()                         const { return this->inventory.GetGold(); };
  uint32_t                  GetGems(Element element)          const { return this->inventory.GetGems(element); };

  // rendering
  virtual IColor            GetLight()                        const;

  virtual const Entity_Proto &GetProto();

  virtual void              SetTrigger(uint32_t id, bool toggle = false) override { this->proto.set_trigger_id(id); this->proto.set_is_trigger_toggle(toggle); }
  virtual uint32_t          GetTriggerId() const override { return this->proto.has_trigger_id() ? this->proto.trigger_id() : InvalidID; }
  virtual bool              IsTriggerToggle() const override { return this->proto.is_trigger_toggle(); }

  virtual void              SetTriggered(bool triggered) override { this->proto.set_is_triggered(triggered); }
  virtual bool              IsTriggered() const override{ return this->proto.is_triggered(); }

  void                      SetRenderAngle(float angle)             { this->proto.set_render_angle(angle); }

protected:

                            Entity(const std::string &type);
                            Entity(const Entity_Proto &proto);

  float                     GetStartTime()                    const { return this->proto.start_time(); }
  void                      SetStartTime(float t)                   { this->proto.set_start_time(t); }

  float                     GetDieTime()                      const { return this->proto.die_time(); }
  void                      SetDieTime(float t)                     { this->proto.set_die_time(t); }

  Entity_Proto proto;

  // management
  bool removable;
  const EntityProperties *properties;

  std::unordered_map<std::string, Regular> regulars;
  std::vector<std::string> queuedSounds;

  // gameplay
  Smooth<Vector3> smoothPosition = Smooth<Vector3>(30.0);

  Stats baseStats;
  std::vector<Buff> activeBuffs;
  AABB aabb;

  Cell *lastCell;
  IVector3 cellPos;
  Inventory inventory;

  // rendering
  Sprite sprite;
  bool drawAABB;
  IColor cellLight;
  std::vector<ParticleEmitter> emitters = std::vector<ParticleEmitter>(0);
};

#endif
