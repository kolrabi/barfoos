#include "common.h"

#include "entity.h"

#include "player.h"
#include "monster.h"
#include "projectile.h"
#include "itementity.h"
#include "item.h"

#include "world.h"
#include "cell.h"
#include "fileio.h"
#include "random.h"

#include "gfx.h"
#include "gfxview.h"
#include "texture.h"

#include "audio.h"
#include "input.h"

#include "runningstate.h"

#include "serializer.h"
#include "deserializer.h"

#include <unordered_map>

static std::unordered_map<std::string, EntityProperties> allEntities;
static std::unordered_map<std::string, std::vector<std::string>> allEntityGroups;
static EntityProperties defaultEntity;

const EntityProperties *getEntity(const std::string &name) {
  if (allEntities.find(name) == allEntities.end()) {
    Log("Properties for entity of type '%s' not found\n", name.c_str());
    return &defaultEntity;
  }
  return &allEntities[name];
}

void
EntityProperties::ParseProperty(const std::string &cmd) {
  if (cmd == "tex")                   Parse("entities/texture/", this->sprite.texture);
  else if (cmd == "class")            Parse(this->klass);
  else if (cmd == "emissivetex")      Parse("entities/texture/", this->sprite.emissiveTexture);
  else if (cmd == "frames")           Parse(this->sprite.totalFrames);
  else if (cmd == "group")            Parse(this->groups);
  else if (cmd == "anim") {
    uint32_t firstFrame = 0;
    uint32_t frameCount = 0;
    float  fps = 0;

    Parse(firstFrame);
    Parse(frameCount);
    Parse(fps);

    this->sprite.animations.push_back(Animation(firstFrame, frameCount, fps));
  } else if (cmd == "size") {
    Parse(this->sprite.width);
    Parse(this->sprite.height);
  } else if (cmd == "sizerand")       Parse(this->sizeRand);

  else if (cmd == "name")             Parse(this->displayName);

  else if (cmd == "flinchanim")       Parse(this->flinchAnim);
  else if (cmd == "dyinganim")        Parse(this->dyingAnim);
  else if (cmd == "attackanim")       Parse(this->attackAnim);

  else if (cmd == "respawn")          this->respawn         = true;
  else if (cmd == "nostep")           this->noStep          = true;
  else if (cmd == "vert")             this->sprite.vertical = true;
  else if (cmd == "alignyforward")    this->alignYForward   = true;

  else if (cmd == "box")              this->isBox           = true;
  else if (cmd == "nohit")            this->nohit           = true;
  else if (cmd == "noitemuse")        this->noItemUse       = true;
  else if (cmd == "solid")            this->isSolid         = true;
  else if (cmd == "nocollideentity")  this->nocollideEntity = true;
  else if (cmd == "nocollidecell")    this->nocollideCell   = true;
  else if (cmd == "nocollideowner")   this->nocollideOwner  = true;
  else if (cmd == "nofriction")       this->noFriction      = true;
  else if (cmd == "oncollideusecell") this->onCollideUseCell = true;
  else if (cmd == "swim")             this->swim = true;
  else if (cmd == "flipleft")         this->flipLeft = true;
  else if (cmd == "bubble")           this->createBubbles   = true;
  else if (cmd == "openinventory")    this->openInventory   = true;
  else if (cmd == "learnevade")       this->learnEvade = true;
  else if (cmd == "randomangle")      this->randomAngle = true;
  else if (cmd == "quad")             this->isQuad = true;

  else if (cmd == "element")          Parse(this->element);

  else if (cmd == "level") {
    Parse(this->minLevel);
    Parse(this->maxLevel);
    Parse(this->maxProbability);
  }
  else if (cmd == "step")             Parse(this->stepHeight);
  else if (cmd == "jump")             Parse(this->jumpSpeed);
  else if (cmd == "mass")             Parse(this->mass);
  else if (cmd == "move")             Parse(this->moveInterval);
  else if (cmd == "speed")            Parse(this->maxSpeed);
  else if (cmd == "health")           Parse(this->maxHealth);
  else if (cmd == "heal")             Parse(this->heal);
  else if (cmd == "lifetime")         Parse(this->lifetime);
  else if (cmd == "lifetimerand")     Parse(this->lifetimeRand);
  else if (cmd == "lockedchance")     Parse(this->lockedChance);
  else if (cmd == "impactdamage")     Parse(this->impactDamage);
  else if (cmd == "extents") {
    Parse(this->extents);
    this->sprite.width = this->extents.x*2.0;
    this->sprite.height = this->extents.y*2.0;
  } else if (cmd == "drawbox") {
    EntityDrawBox box;
    Parse("entities/texture/", box.texture);
    Parse("entities/texture/", box.emissiveTexture);
    Parse(box.aabb.center);
    Parse(box.aabb.extents);
    this->drawBoxes.push_back(box);

  } else if (cmd == "emitter") {
    ParticleEmitter emitter;
    Parse(emitter.name);
    Parse(emitter.rate);
    Parse(emitter.velocity);
    Parse(emitter.aabb.center);
    Parse(emitter.aabb.extents);
    emitter.state = 0.0;
    this->emitters.push_back(emitter);

  } else if (cmd == "keepdistance") {
    Parse(this->keepDistance);
  } else if (cmd == "aggro") {
    this->aggressive      = true;
    Parse(this->attackInterval);
    Parse(this->aggroRangeNear);
    Parse(this->aggroRangeFar);
    Parse(this->meleeAttackRange);
    Parse(this->attackItem);

  } else if (cmd == "retaliate") {
    this->retaliate      = true;
    Parse(this->attackInterval);
    Parse(this->aggroRangeNear);
    Parse(this->aggroRangeFar);
    Parse(this->meleeAttackRange);
    Parse(this->attackItem);

  } else if (cmd == "gravity")        Parse(this->gravity);
  else if (cmd == "attackforwardstep") Parse(this->attackForwardStep);
  else if (cmd == "attackjump")       Parse(this->attackJump);
  else if (cmd == "eyeoffset")        Parse(this->eyeOffset);
  else if (cmd == "glow")             Parse(this->glow);
  else if (cmd == "exp")              Parse(this->exp);
  else if (cmd == "str")              Parse(this->str);
  else if (cmd == "dex")              Parse(this->dex);
  else if (cmd == "agi")              Parse(this->agi);
  else if (cmd == "def")              Parse(this->def);
  else if (cmd == "mdef")             Parse(this->mdef);
  else if (cmd == "matk")             Parse(this->matk);
  else if (cmd == "thinkinterval")    Parse(this->thinkInterval);
  else if (cmd == "onuseitemreplace") {
    std::pair<std::string, std::string> replace;
    Parse(replace.first);
    Parse(replace.second);
    this->onUseItemReplace[replace.first] = replace.second;

  } else if (cmd == "onuseentityreplace") {
    std::pair<std::string, std::string> replace;
    Parse(replace.first);
    Parse(replace.second);
    this->onUseEntityReplace[replace.first] = replace.second;

  } else if (cmd == "ondieexplode") {
    Parse(this->onDieExplodeRadius);
    Parse(this->onDieExplodeStrength);
    Parse(this->onDieExplodeDamage);
    Parse(this->onDieExplodeElement);

  } else if (cmd == "ondieexplodeaddbuff") {
    Parse(this->onDieExplodeAddBuff);

  } else if (cmd == "magical") {
    this->onDieExplodeMagical = true;

  } else if (cmd == "sound") {
    Parse(this->sounds);
  } else if (cmd == "ondieparticles") {
    Parse(this->onDieParticles);
    Parse(this->onDieParticleSpeed);
    Parse(this->onDieParticleType);

  } else if (cmd == "inventory") {
    float prob;
    std::string type;
    Parse(prob);
    Parse(type);

    this->items.push_back({type, prob});
  } else if (cmd == "cell") {
    Parse(this->cellEnter);
    Parse(this->cellLeave);

  }
  else if (cmd != "") {
    this->SetError("Ignoring '" + cmd + "'");;
  }
}

void
LoadEntities() {
  std::vector<std::string> assets = findAssets("entities");
  for (const std::string &name : assets) {
    FILE *f = openAsset("entities/"+name);
    if (f) {
      allEntities[name].name = name;
      allEntities[name].groups.push_back(name);
      allEntities[name].ParseFile(f);
      for (auto &g : allEntities[name].groups) {
        allEntityGroups[g].push_back(name);
      }
      fclose(f);
    }
  }
}

const std::vector<std::string> &
GetEntitiesInGroup(const std::string &group) {
  return allEntityGroups[group];
}

float GetEntityProbability(const std::string &name, int level) {
  if (allEntities.find(name) == allEntities.end()) return 0.0;

  const EntityProperties &prop = allEntities[name];

  if (level < prop.minLevel) return 0;
  if (prop.maxLevel < prop.minLevel) return prop.maxProbability;

  // make highest chance right between min and max level
  float levelFrac = (level-prop.minLevel)/(float)(prop.maxLevel-prop.minLevel + 1);
  return prop.maxProbability * std::sin(Const::pi*levelFrac);
}

Entity *Entity::Create(const std::string &type) {
  //if (allEntities.find(type) == allEntities.end()) return nullptr;
  const EntityProperties &prop = allEntities[type];

  Entity *entity;
  switch(prop.klass) {
    case SpawnClass::EntityClass:     entity = new Entity(type); break;
    case SpawnClass::MobClass:        entity = new Mob(type);    break;
    case SpawnClass::MonsterClass:    entity = new Monster(type);    break;
    case SpawnClass::ItemEntityClass: entity = new ItemEntity(type); break;
    case SpawnClass::PlayerClass:     entity = new Player(); break;
    case SpawnClass::ProjectileClass: entity = new Projectile(type); break;
    default: entity = nullptr;
  }
  return entity;
}
/*
Entity *Entity::Create(const std::string &type, Deserializer &deser) {
  //if (allEntities.find(type) == allEntities.end()) return nullptr;
  const EntityProperties &prop = allEntities[type];

  Entity *entity;
  switch(prop.klass) {
    case SpawnClass::EntityClass:     entity = new Entity(type, deser); break;
    case SpawnClass::MobClass:        entity = new Mob(type, deser);    break;
    case SpawnClass::MonsterClass:    entity = new Monster(type, deser);    break;
    case SpawnClass::ItemEntityClass: entity = new ItemEntity(deser); break;
    case SpawnClass::PlayerClass:     entity = new Player(deser); break;
    case SpawnClass::ProjectileClass: entity = new Projectile(type, deser); break;
    default: entity = nullptr;
  }
  return entity;
}
*/
Entity::Entity(const std::string &type) :
  removable(false),
  properties(getEntity(type)),
  regulars(),
  baseStats(),
  aabb(this->properties->extents),
  lastCell(nullptr),
  cellPos(),
  inventory(),
  sprite(this->properties->sprite),
  drawAABB(false),
  cellLight(0,0,0),
  emitters(this->properties->emitters)
{
  this->proto.set_spawn_class(uint32_t(SpawnClass::EntityClass));
}

Entity::Entity(const Entity_Proto &proto) :
  proto(proto),
  removable(false),
  properties(getEntity(proto.type())),
  regulars(),
  baseStats(),
  aabb(this->properties->extents),
  lastCell(nullptr),
  cellPos(),
  inventory(),
  sprite(this->properties->sprite),
  drawAABB(false),
  cellLight(0,0,0),
  emitters(this->properties->emitters)
{
}

Entity::~Entity() {
}

void
Entity::Start(RunningState &state, uint32_t id) {
  Game &game = state.GetGame();
  World &world = state.GetWorld();

  this->proto.set_id(id);
  this->proto.set_next_think_time(game.GetTime());
  this->proto.set_start_time(game.GetTime());

  if (this->properties->randomAngle) {
    this->proto.set_render_angle(state.GetRandom().Float01() * 360.0);
  }

  if (this->properties->lifetime) {
    this->SetDieTime(game.GetTime() + this->properties->lifetime + state.GetRandom().Float() * this->properties->lifetimeRand);
  }

  float f = 1.0 + state.GetRandom().Float() * this->properties->sizeRand;
  this->sprite.width *= f;
  this->sprite.height *= f;

  // fill inventory with random crap
  for (auto item : this->properties->items) {
    if (game.GetRandom().Chance(item.second)) {
      Item *ii;
      if (item.first[0] == '$') {
        std::string itemName = getRandomItem(item.first.substr(1), state.GetLevel(), state.GetRandom());
        ii = new Item(itemName);
      } else {
        ii = new Item(item.first);
      }
      this->GetInventory().AddToBackpack(std::shared_ptr<Item>(ii));
    }
  }

  // set stats
  this->baseStats.SetStrength(this->properties->str);
  this->baseStats.SetDexterity(this->properties->dex);
  this->baseStats.SetAgility(this->properties->agi);
  this->baseStats.SetDefense(this->properties->def);
  this->baseStats.SetMagicDefense(this->properties->mdef);
  this->baseStats.SetMagicAttack(this->properties->matk);
  this->baseStats.SetMaxHealth(this->properties->maxHealth);
  this->proto.set_health(this->properties->maxHealth);

  // resolve initial collision with world
  std::vector<Vector3> verts;
  verts.push_back(Vector3(-aabb.extents.x, -aabb.extents.y, -aabb.extents.z));
  verts.push_back(Vector3( aabb.extents.x, -aabb.extents.y, -aabb.extents.z));
  verts.push_back(Vector3(-aabb.extents.x, -aabb.extents.y,  aabb.extents.z));
  verts.push_back(Vector3( aabb.extents.x, -aabb.extents.y,  aabb.extents.z));
  verts.push_back(Vector3(-aabb.extents.x,  aabb.extents.y, -aabb.extents.z));
  verts.push_back(Vector3( aabb.extents.x,  aabb.extents.y, -aabb.extents.z));
  verts.push_back(Vector3(-aabb.extents.x,  aabb.extents.y,  aabb.extents.z));
  verts.push_back(Vector3( aabb.extents.x,  aabb.extents.y,  aabb.extents.z));

  size_t sides = 0;
  for (Vector3 v : verts) {
    if (world.IsPointSolid(aabb.center + v)) {
      if (v.x < 0) sides |= (size_t)Side::Left;
      if (v.x > 0) sides |= (size_t)Side::Right;
      if (v.y < 0) sides |= (size_t)Side::Down;
      if (v.y > 0) sides |= (size_t)Side::Up;
      if (v.z < 0) sides |= (size_t)Side::Backward;
      if (v.z > 0) sides |= (size_t)Side::Forward;
    }
  }
  Vector3 offset;
  for (int i=0; i<6; i++) {
    if (sides & (1<<i)) {
      Vector3 d((Side)i);
      offset = offset - d * d.Dot(aabb.extents);
    }
  }
  this->SetPosition(aabb.center + offset);

  if (this->properties->lockedChance && state.GetRandom().Chance(this->properties->lockedChance)) state.LockEntity(*this);

  this->PlaySound(state, "start");
}

void
Entity::Continue(RunningState &, uint32_t id) {
  this->proto.set_id(id);

  this->aabb.center = Vector3(this->proto.position().x(), this->proto.position().y(), this->proto.position().z());
  this->aabb.extents = this->properties->extents;

  this->activeBuffs.clear();
  for (auto &b : this->proto.active_buffs()) {
    this->activeBuffs.push_back(Buff(b));
  }

  this->baseStats = Stats(this->proto.base_stats());

  this->triggerID = this->proto.trigger_id();
  this->isToggle = this->proto.is_toggle();
  this->triggered = this->proto.is_triggered();

  this->inventory.Clear();
  for (auto &i:this->proto.inventory()) {
    this->inventory[InventorySlot(i.slot())] = std::shared_ptr<Item>(new Item(i.item()));
  }
}

void
Entity::Update(RunningState &state) {
  Game &game   = state.GetGame();
  float deltaT = game.GetDeltaT();
  float t      = game.GetTime();
  PROFILE();

  // play queued sounds
  for (auto &s:this->queuedSounds)
    this->PlaySound(state, s);
  this->queuedSounds.clear();

  if (this->GetDieTime() && state.GetGame().GetTime() > this->GetDieTime()) {
    this->Die(state, HealthInfo());
  }

  // bring out your dead
  if (this->IsDead() && this->sprite.currentAnimation == 0) {
    if (this->properties->respawn) {
      // just respawn
      this->proto.set_is_dead(false);
      SetPosition(this->GetSpawnPosition());
      Start(state, this->GetId());
      Log("respawning %s\n", this->properties->name.c_str());
    } else {
      //Log("marking %s as removable\n", this->properties->name.c_str());
      this->removable = true;
    }
    return;
  }

  for (auto &r:this->regulars) r.second.Update(deltaT);
  this->sprite.Update(deltaT);

  // think, mcfly, think
  while(properties->thinkInterval && this->proto.next_think_time() < t) {
    this->proto.set_next_think_time(this->proto.next_think_time() + properties->thinkInterval);
    Think(state);
  }

  this->inventory.Update(state, *this);
  this->smoothPosition.Update(deltaT);

  if (!this->IsDead()) {
    this->proto.set_health(
      std::min(  
        this->GetHealth() + deltaT * this->properties->heal, 
        (float)this->GetEffectiveStats().GetMaxHealth()
      )
    );

    for (auto &e:emitters) {
      e.state += e.rate * deltaT;

      for (int n = 0; n<int(e.state); n++) {
        Mob *particle = new Mob(e.name);
        Vector3 p = state.GetRandom().Vector() * e.aabb.extents + e.aabb.center + this->aabb.center;
        particle->SetPosition(p);
        particle->AddVelocity(e.velocity);
        state.AddEntity(particle);
      }

      e.state -= int(e.state);
    }

    auto it = this->activeBuffs.begin();
    while(it != this->activeBuffs.end()) {
      if (t > it->GetEffect().duration + it->GetStartTime()) {
        state.GetGame().GetAudio().PlaySound(it->GetEffect().removeSound, this->GetPosition());
        it = this->activeBuffs.erase(it);
      } else {
        it->GetEffect().Update(state, *this);
        if (this->IsDead()) return;
        it++;
      }
    }
  }

  this->cellPos = IVector3(aabb.center.x, aabb.center.y, aabb.center.z);

  World &world = state.GetWorld();
  Cell &cell = world.GetCell(cellPos);
  if (&cell != this->lastCell) {
    if (this->lastCell && this->properties->cellLeave != "") {
      world.SetCell(this->lastCell->GetPosition(), Cell(this->properties->cellLeave));
    }
    if (this->properties->cellEnter != "") {
      world.SetCell(cellPos, Cell(this->properties->cellEnter));
    }

    this->lastCell = &cell;
  } else if (cell.GetInfo().type != this->properties->cellEnter && this->properties->cellLeave != "") {
    world.SetCell(cellPos, Cell(this->properties->cellEnter));
  }

  this->cellLight = world.GetLight(this->GetPosition());

  this->drawAABB = game.GetInput().IsKeyActive(InputKey::DebugEntityAABB);
}

void
Entity::Think(RunningState &) {
}

void
Entity::Draw(Gfx &gfx) const {
  if (!gfx.GetView().IsAABBVisible(this->aabb)) return;

  gfx.SetLight(this->cellLight + this->GetLight());
  gfx.SetColor(IColor(255,255,255), 1.0);
  gfx.SetBlendNormal();

  for (auto &box : this->properties->drawBoxes) {
    AABB aabb = box.aabb;
    aabb.center = aabb.center + this->GetSmoothPosition();

    gfx.SetTextureFrame(box.texture,0,0,8);
    gfx.DrawAABB(aabb);

    if (box.emissiveTexture) {
      gfx.SetBlendAdd();
      gfx.SetTextureFrame(box.texture,0,0,8);
      gfx.DrawAABB(aabb);
      gfx.SetBlendNormal();
    }
  }

  if (this->properties->isBox) {
    gfx.GetView().Push();
    gfx.GetView().Rotate(this->proto.render_angle(), Vector3(0,1,0));
    if (this->properties->sprite.texture) {
      gfx.SetTextureFrame(this->properties->sprite.texture,0,0,8);
      gfx.DrawAABB(this->aabb);
    }
    if (this->properties->sprite.emissiveTexture) {
      gfx.SetBlendAdd();
      gfx.SetLight(IColor());
      gfx.SetTextureFrame(this->properties->sprite.emissiveTexture,0,0,8);
      gfx.DrawAABB(this->aabb);
      gfx.SetBlendNormal();
    }
    gfx.GetView().Pop();
  } else {
    if (this->properties->alignYForward) {
      gfx.DrawSprite(this->sprite, this->aabb.center, this->GetForward());
    } else {
      gfx.DrawSprite(this->sprite, this->aabb.center, this->properties->flipLeft && gfx.GetView().GetRight().Dot(GetForward())<0, !this->properties->isQuad, this->proto.render_angle());
    }
  }

  if (this->drawAABB) {
    this->DrawBoundingBox(gfx);
  }
}

void
Entity::DrawBoundingBox(Gfx &gfx) const {
  gfx.SetTextureFrame(gfx.GetNoiseTexture());
  gfx.SetColor(IColor(64,64,64),0);
  gfx.DrawAABB(this->aabb);
}

void
Entity::AddHealth(RunningState &state, const HealthInfo &info) {
  // don't change health of dead or immortal entities
  if (this->GetHealth() <= 0 || this->properties->maxHealth == 0) return;

  this->proto.set_health(this->GetHealth() + info.amount);

  // Log("Entity::AddHealth: %f to %u\n", info.amount, this->GetId());

  Entity *dealer = state.GetEntity(info.dealerId);
  if (dealer) dealer->OnHealthDealt(state, *this, info);

  if (info.amount < 0) {
    this->sprite.StartAnim(this->properties->flinchAnim);
    this->sprite.QueueAnim(0);
  }

  if (this->GetHealth() <= 0 && info.amount < 0.0) {
    this->proto.set_health(0);
    this->Die(state, info);
  } else if (this->GetHealth() > this->GetEffectiveStats().GetMaxHealth()) {
    this->proto.set_health(this->GetEffectiveStats().GetMaxHealth());
  }
}

void
Entity::Die(RunningState &state, const HealthInfo &info) {
  this->proto.set_is_dead(true);

  // Log("Entity::Die: from %f damage to %u\n", info.amount, this->GetId());

  this->PlaySound(state, "death");

  this->activeBuffs.clear();

  if (info.dealerId != InvalidID && state.GetEntity(info.dealerId)) {
    state.GetPlayer().AddDeathMessage(*this, *state.GetEntity(info.dealerId), info);
  } else {
    state.GetPlayer().AddDeathMessage(*this, info);
  }

  if (this->lastCell && this->properties->cellLeave != "") {
    state.GetWorld().SetCell(this->lastCell->GetPosition(), Cell(this->properties->cellLeave));
  }

  if (this->properties->onDieExplodeRadius) {
    state.Explosion(*this, this->GetPosition(), this->properties->onDieExplodeRadius, this->properties->onDieExplodeStrength, this->properties->onDieExplodeDamage, this->properties->onDieExplodeElement, this->properties->onDieExplodeMagical);
    if (this->properties->onDieExplodeAddBuff != "") {
      std::vector<ID> ents = state.FindEntities(this->GetPosition(), this->properties->onDieExplodeRadius);
      for (ID eid:ents) {
        Entity *ent = state.GetEntity(eid);
        ent->AddBuff(state, this->properties->onDieExplodeAddBuff);
      }
    }
  }

  if (this->properties->onDieParticles) {
    for (size_t i=0; i<this->properties->onDieParticles; i++)
      state.SpawnInAABB(this->properties->onDieParticleType, this->aabb, state.GetRandom().Vector()*this->properties->onDieParticleSpeed);
  }

  this->inventory.Drop(state, *this);

  if (this->properties->dyingAnim != InvalidID) {
    this->sprite.StartAnim(this->properties->dyingAnim);
    this->sprite.QueueAnim(0);
  } else {
    this->sprite.StartAnim(0);
  }
}

Stats
Entity::GetEffectiveStats() const {
  Stats stats = this->baseStats;
  this->inventory.ModifyStats(stats);
  for (auto &b : this->activeBuffs) {
    b.GetEffect().ModifyStats(stats, true, 0, Beatitude::Normal); // TODO: modifiers for buffs (aka minor, major, ...)
  }
  return stats;
}

void
Entity::OnHealthDealt(RunningState &state, Entity &, const HealthInfo &info) {
  if (this->baseStats.AddExperience(info.exp)) this->OnLevelUp(state);
}

void
Entity::AddBuff(RunningState &state, const std::string &name) {
  if (name == "") return;

  Buff buff(name, state.GetGame().GetTime());

  state.GetGame().GetAudio().PlaySound(buff.GetEffect().addSound, this->GetPosition());

  for (auto &b:this->activeBuffs) {
    if (b.GetEffect() == buff.GetEffect()) {
      if (b.GetEffect().extend) {
        b.Extend(b.GetEffect().duration);
      } else {
        b.Restart(state.GetGame().GetTime());
      }
      return;
    }
  }

  this->activeBuffs.push_back(buff);
  this->OnBuffAdded(state, buff.GetEffect());
}

bool
Entity::CanSee(RunningState &state, const Vector3 &pos) {
  Vector3 start = GetPosition()+Vector3(0,properties->eyeOffset,0);
  Vector3 dir = pos - start;

  float dist;
  Side side;

  state.GetWorld().CastRayCell(start, dir.Normalize(), dist, side);
  return dist >= dir.GetMag();
}

void
Entity::Teleport(RunningState &state, const Vector3 &target) {
  AABB aabb = GetAABB();
  aabb.extents.x += 0.5;
  aabb.extents.z += 0.5;
  for (size_t i=0; i<5; i++) {
    state.SpawnInAABB("particle.teleport", aabb, Vector3(0, state.GetRandom().Float()*0.3, 0));
  }
  SetPosition(Vector3(0.5, 1.0 + aabb.extents.y, 0.5) + Vector3(target));
  aabb.center = GetPosition();
  for (size_t i=0; i<25; i++) {
    state.SpawnInAABB("particle.teleport", aabb, Vector3(0, state.GetRandom().Float()*0.3, 0));
  }
}

void
Entity::PlaySound(RunningState &state, const std::string &type) {
  if (this->properties->sounds.find(type) == this->properties->sounds.end()) return;
  state.GetGame().GetAudio().PlaySound(this->properties->sounds.at(type), GetSmoothPosition());
}

void
Entity::PlaySound(const std::string &type) {
  this->queuedSounds.push_back(type);
}

IColor
Entity::GetLight() const {
  IColor buffLight;
  for (auto &b : this->activeBuffs) {
    buffLight = buffLight + b.GetEffect().light;
  }
  return this->properties->glow + inventory.GetLight() + buffLight;
}

const Entity_Proto &
Entity::GetProto() {
  this->proto.mutable_position()->set_x(this->aabb.center.x);
  this->proto.mutable_position()->set_y(this->aabb.center.y);
  this->proto.mutable_position()->set_z(this->aabb.center.z);

  this->proto.clear_active_buffs();
  for (auto &b : this->activeBuffs) {
    *this->proto.add_active_buffs() = b.proto;
  }

  this->proto.clear_inventory();
  for (uint8_t s=0; s<uint8_t(InventorySlot::End); s++) {
    if (this->inventory[InventorySlot(s)]) {
      Inventory_Proto *p = this->proto.add_inventory();
      p->set_slot(s);
      *p->mutable_item() = this->inventory[InventorySlot(s)]->GetProto();
    }
  }

  *this->proto.mutable_base_stats() = this->baseStats.proto;

  this->proto.set_trigger_id(this->triggerID);
  this->proto.set_is_toggle(this->isToggle);
  this->proto.set_is_triggered(this->triggered);

  return this->proto;
}
