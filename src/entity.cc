#include "entity.h"
#include "world.h"
#include "cell.h"
#include "util.h"
#include "item.h"
#include "itementity.h"
#include "random.h"
#include "gfx.h"
#include "input.h"
#include "texture.h"
#include "player.h"
#include "runningstate.h"
#include "serializer.h"

#include <unordered_map>

static std::unordered_map<std::string, EntityProperties> allEntities;
EntityProperties defaultEntity;

const EntityProperties *getEntity(const std::string &name) {
  if (allEntities.find(name) == allEntities.end()) {
    Log("Properties for entity of type '%s' not found\n", name.c_str());
    return &defaultEntity;
  }
  return &allEntities[name];
}

void
EntityProperties::ParseProperty(const std::string &cmd) {
  if (cmd == "tex")             Parse("entities/texture/", this->sprite.texture);
  else if (cmd == "frames")     Parse(this->sprite.totalFrames);
  else if (cmd == "anim") {
    size_t firstFrame = 0;
    size_t frameCount = 0;
    float  fps = 0;
    
    Parse(firstFrame);
    Parse(frameCount);
    Parse(fps);
    
    this->sprite.animations.push_back(Animation(firstFrame, frameCount, fps));
  } else if (cmd == "size") {
    Parse(this->sprite.width);
    Parse(this->sprite.height);
      
  } else if (cmd == "name")     Parse(this->displayName);
    
  else if (cmd == "flinchanim") Parse(this->flinchAnim);
  else if (cmd == "dyinganim")  Parse(this->dyingAnim);
      
  else if (cmd == "respawn")          this->respawn         = true;
  else if (cmd == "vert")             this->sprite.vertical = true;

  else if (cmd == "box")              this->isBox           = true;
  else if (cmd == "nohit")            this->nohit           = true;
  else if (cmd == "solid")            this->isSolid         = true;
  else if (cmd == "nocollideentity")  this->nocollideEntity = true;
  else if (cmd == "nocollidecell")    this->nocollideCell   = true;
  else if (cmd == "nocollideowner")   this->nocollideOwner  = true;
  else if (cmd == "nofriction")       this->noFriction      = true;

  else if (cmd == "step")             Parse(this->stepHeight);
  else if (cmd == "mass")             Parse(this->mass);
  else if (cmd == "move")             Parse(this->moveInterval);
  else if (cmd == "speed")            Parse(this->maxSpeed);
  else if (cmd == "health")           Parse(this->maxHealth);
  else if (cmd == "extents") {
    Parse(this->extents);
    this->sprite.width = this->extents.x;
    this->sprite.height = this->extents.y;
  } else if (cmd == "drawbox") {
    EntityDrawBox box;
    Parse("entities/texture/", box.texture);
    Parse(box.aabb.center);
    Parse(box.aabb.extents);
    this->drawBoxes.push_back(box);
  
  } else if (cmd == "gravity")        Parse(this->gravity);
  else if (cmd == "eyeoffset")        Parse(this->eyeOffset);
  else if (cmd == "glow")             Parse(this->glow);
  else if (cmd == "exp")              Parse(this->exp);
  else if (cmd == "str")              Parse(this->str);
  else if (cmd == "dex")              Parse(this->dex);
  else if (cmd == "agi")              Parse(this->agi);
  else if (cmd == "def")              Parse(this->def);
  else if (cmd == "thinkinterval")    Parse(this->thinkInterval);
  else if (cmd == "onuseitemreplace") {
    std::pair<std::string, std::string> replace;
    Parse(replace.first);
    Parse(replace.second);
    this->onUseItemReplace[replace.first] = replace.second;
    
  } else if (cmd == "inventory") {
    float prob;
    std::string type;
    Parse(prob);
    Parse(type);
    
    this->items.push_back({type, prob});
  } else if (cmd == "cell") {
    Parse(this->cellEnter);
    Parse(this->cellLeave);
  } else if (cmd != "") {
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
      allEntities[name].group = name;
      allEntities[name].ParseFile(f);
      fclose(f);
    }
  }
}

std::vector<std::string>
GetEntitiesInGroup(const std::string &group) {
  std::vector<std::string> entities;
  for (auto &ent : allEntities) {
    if (ent.second.group == group)
      entities.push_back(ent.first);
  }
  return entities;
}

Entity::Entity(const std::string &type) :
  id(~0UL),
  ownerId(~0UL),
  removable(false),
  properties(getEntity(type)),
  nextThinkT(0.0),
  startT(0.0),
  lastPos(),
  spawnPos(),
  angles(),
  baseStats(),
  aabb(this->properties->extents),
  health(this->properties->maxHealth),
  lastCell(nullptr),
  cellPos(),
  inventory(),
  sprite(this->properties->sprite),
  drawAABB(false),
  cellLight(0,0,0)
{
}

Entity::~Entity() {
}

void
Entity::Start(RunningState &state, size_t id) {
  Game &game = state.GetGame();
  World &world = state.GetWorld();
  
  this->id = id;
  this->nextThinkT = game.GetTime();
  this->startT = game.GetTime();
  
  // fill inventory with random crap
  for (auto item : this->properties->items) {
    if (game.GetRandom().Chance(item.second)) {
      Item *ii = new Item(item.first);
      this->GetInventory().AddToBackpack(std::shared_ptr<Item>(ii));
    }
  }

  // set stats
  this->baseStats.str = this->properties->str;
  this->baseStats.dex = this->properties->dex;
  this->baseStats.agi = this->properties->agi;
  this->baseStats.def = this->properties->def;
  this->baseStats.maxHealth = this->properties->maxHealth;
  this->health = this->properties->maxHealth;
  
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
}

void 
Entity::Update(RunningState &state) {
  Game &game   = state.GetGame();
  float deltaT = game.GetDeltaT();
  float t      = game.GetTime();
  
  // bring out your dead
  if (this->IsDead() && this->sprite.currentAnimation == 0) {
    if (this->properties->respawn) {
      // just respawn
      SetPosition(spawnPos);
      Start(state, id);
    } else {
      this->removable = true;
    }
    return;
  }

  // think, mcfly, think
  while(properties->thinkInterval && nextThinkT < t) {
    nextThinkT += properties->thinkInterval;
    Think(state);
  }

  this->sprite.Update(deltaT);
  this->inventory.Update(state, *this);
  this->smoothPosition.Update(deltaT);
  
  auto it = this->activeBuffs.begin();
  while(it != this->activeBuffs.end()) {
    if (t > it->effect->duration + it->startT) {
      it = this->activeBuffs.erase(it);
    } else {
      it->effect->Update(state, *this);
      it++;
    }
  }
  
  this->lastPos = this->aabb.center;
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
  
  this->cellLight = world.GetLight(cell.GetPosition());
  
  this->drawAABB = game.GetInput().IsKeyActive(InputKey::DebugEntityAABB);
}
  
void 
Entity::Think(RunningState &) {
}

void
Entity::Draw(Gfx &gfx) const {
  gfx.SetColor(this->cellLight + this->GetLight(), 1.0);
  
  for (auto &box : this->properties->drawBoxes) {
    AABB aabb = box.aabb;
    aabb.center = aabb.center + this->GetSmoothPosition();
    
    gfx.SetTextureFrame(box.texture,0,0,8);
    gfx.DrawAABB(aabb);
  }
  
  if (this->properties->isBox) {
    gfx.SetTextureFrame(this->properties->sprite.texture,0,0,8);
    gfx.DrawAABB(this->aabb);
  } else {
    gfx.DrawSprite(this->sprite, this->aabb.center);
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
  if (this->health <= 0 || this->properties->maxHealth == 0) return;
  
  this->health += info.amount;

  Entity *dealer = state.GetEntity(info.dealerId);
  if (dealer) dealer->OnHealthDealt(state, *this, info);
  
  if (info.amount < 0) {
    this->sprite.StartAnim(this->properties->flinchAnim);
    this->sprite.QueueAnim(0);
  }
  
  if (this->health <= 0) {
    this->health = 0;
    this->Die(state, info);
  }
}

void
Entity::Die(RunningState &state, const HealthInfo &info) {
  if (info.dealerId != ~0UL) {
    state.GetPlayer().AddDeathMessage(*this, *state.GetEntity(info.dealerId), info);
  } else {
    state.GetPlayer().AddDeathMessage(*this, info);
  }
  
  if (this->lastCell && this->properties->cellLeave != "") {
    state.GetWorld().SetCell(this->lastCell->GetPosition(), Cell(this->properties->cellLeave));
  }

  this->inventory.Drop(state, *this);
  this->sprite.StartAnim(this->properties->dyingAnim);
  this->sprite.QueueAnim(0);
}

Stats 
Entity::GetEffectiveStats() const {
  Stats stats = this->baseStats;
  this->inventory.ModifyStats(stats);
  for (auto &b : this->activeBuffs) {
    b.effect->ModifyStats(stats, true);
  }
  return stats;
}

void
Entity::OnHealthDealt(RunningState &state, Entity &, const HealthInfo &info) {
  if (this->baseStats.AddExp(info.exp)) this->OnLevelUp(state);
}

void
Entity::AddBuff(RunningState &state, const std::string &name) {
  Buff buff;
  buff.effect = &getEffect(name);
  buff.startT = state.GetGame().GetTime();
  this->activeBuffs.push_back(buff);
}

void 
Entity::Serialize(Serializer &ser) const {
  ser << ownerId;
  ser << nextThinkT;
  ser << startT;
  ser << lastPos << spawnPos << angles;
  
  ser << baseStats;
  ser << activeBuffs;
  ser << aabb;
  
  ser << health;
  ser << inventory;
  //ser << sprite;
}

Serializer &operator << (Serializer &ser, const Entity *entity) {
  ser << (char)entity->GetSpawnClass();
  ser << entity->properties->name;
  entity->Serialize(ser);
  return ser;
}
