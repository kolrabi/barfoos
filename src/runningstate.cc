#include "runningstate.h"
#include "world.h"
#include "ivector3.h"
#include "world.h"
#include "player.h"
#include "worldedit.h"
#include "feature.h"
#include "inventorygui.h"
#include "gfx.h"
#include "input.h"

#include "serializer.h"
#include "deserializer.h"

#include <algorithm>
#include <sys/time.h>

RunningState::RunningState(Game &game) :
  GameState(game),
  level(0),
  world(nullptr),
  nextEntityId(1),
  player(nullptr),
  showInventory(false),
  lastSaveT(0.0),
  saving(false)
{
  Log("+RunningState() %p %p %p %p\n", this, &GetRandom(), &game, &GetGame());
}

RunningState::~RunningState() {
  for (auto entity : this->entities) {
    delete entity.second;
  }
  
  Log("-RunningState()\n");
}

void
RunningState::Enter() {
}

void
RunningState::Leave(GameState *) {
  this->Save();
}

void
RunningState::NewGame() {
  this->level = 0;

  std::string seed = ToString(time(nullptr));
  GetGame().NewGame(seed);

  delete this->world;
  this->world = new World(*this, IVector3(64, 64, 64));
  this->world->Build();

  Player *player = new Player();
  player->SetPosition(IVector3(32,32,32));
  player->SetSpawnPos(IVector3(32,32,32));
  this->AddPlayer(player);
  
  Entity *entity = new Entity("box");
  entity->SetPosition(IVector3(32,24,32));
  this->AddEntity(entity);
}

void
RunningState::ContinueGame() {
  Load();
}

void
RunningState::Render(Gfx &gfx) const {
  PROFILE();
  
  std::vector<IColor> lightColors;
  std::vector<Vector3> lightPositions;
  std::vector<const Entity*> lightEntities = FindLightEntities(player->GetSmoothPosition(), 32);
  for (auto e : lightEntities) {
    lightColors.push_back(e->GetLight());
    lightPositions.push_back(e->GetSmoothEyePosition());
  }
  
  gfx.ClearColor(IColor(30, 30, 20));
  gfx.ClearDepth(1.0);
  gfx.Viewport(Rect());

  // draw world first
  gfx.SetFog(0.05, IColor(64,64,64));
  gfx.SetLights(lightPositions, lightColors);
  
  player->View(gfx);
  world->Draw(gfx);
  
  // draw all entities
  for (auto entity : this->entities) {
    entity.second->Draw(gfx);
  }
  
  gfx.ClearDepth(1.0);
  player->DrawWeapons(gfx);
  
  player->MapView(gfx);
  world->DrawMap(gfx);

  // next draw gui stuff
  gfx.GetView().GUI();
  
  player->DrawGUI(gfx);
}

GameState * 
RunningState::Update() {
  PROFILE();

  // show or hide inventory
  if (GetGame().GetInput().IsKeyActive(InputKey::Inventory)) {
    if (!this->showInventory) {
      size_t ent = this->player->GetSelectedEntity();
      if (ent == ~0UL /*|| !entities[ent]->GetProperties()->CanOpenInventory() TODO */) {
        GetGame().SetGui(std::shared_ptr<Gui>(new InventoryGui(*this, *player)));
      } else {
        GetGame().SetGui(std::shared_ptr<Gui>(new InventoryGui(*this, *player, *entities[ent])));
      }
    }
    this->showInventory = true;
  } else {
    if (this->showInventory) {
      GetGame().SetGui(nullptr);
    }
    this->showInventory = false;
  }
  
  // update world
  this->world->Update(*this);
  
  // handle collision between entities
  std::vector<size_t> entityIds;
  for (auto entity : this->entities) {
    entityIds.push_back(entity.first);
  }
  
  for (size_t i=0; i<entityIds.size(); i++) {
    size_t e1 = entityIds[i];
    if (entities[e1]->GetProperties()->nocollideEntity) continue;
    if (entities[e1]->IsDead()) continue;

    for (size_t j=i+1; j<entityIds.size(); j++) {
      size_t e2 = entityIds[j];
      if (entities[e2]->GetProperties()->nocollideEntity) continue;
      if (entities[e2]->IsDead()) continue;
      
      // don't collide with owners if not wanted
      if (entities[e1]->GetOwner() == e2 && entities[e1]->GetProperties()->nocollideOwner) continue;
      if (entities[e2]->GetOwner() == e1 && entities[e2]->GetProperties()->nocollideOwner) continue;
      
      if (entities[e1]->GetAABB().Overlap(entities[e2]->GetAABB())) {
        entities[e1]->OnCollide(*this, *entities[e2]);
        entities[e2]->OnCollide(*this, *entities[e1]);
      }
    }
  }

  // update all entities
  {
    PROFILE_NAMED("update"); 
    for (auto entity : this->entities) {
      entity.second->Update(*this); 
    }
  }
  
  // remove removable entities
  {
    PROFILE_NAMED("remove"); 
    auto entityIter = this->entities.begin();
    while(entityIter != this->entities.end()) {
      if (!entityIter->second || entityIter->second->IsRemovable()) {
//        Log("removing %s\n", entityIter->second->GetProperties()->name.c_str());
        delete entityIter->second;
        entityIter = this->entities.erase(entityIter);
      } else {
        entityIter++;
      }
    }
  }
  
/*  if (GetGame().GetTime() > this->lastSaveT + 5.0) {
    this->lastSaveT += 5.0;
    Save();
  }
  */
  return this;
}

void 
RunningState::BuildWorld() { 
}

void RunningState::HandleEvent(const InputEvent &event) {
  if (this->player) this->player->HandleEvent(event);
}

/**
 * Add an entity to this game.
 * @param entity Entity to add
 */
size_t
RunningState::AddEntity(Entity *entity) {
  size_t entityId = GetNextEntityId();
  this->entities[entityId] = entity;
  entity->Start(*this, entityId);
  return entityId;
}

/**
 * Add the player entity to this game. 
 * Also stores the player for future reference.
 * @param player Player to add
 */
size_t
RunningState::AddPlayer(Player *player) {
  this->player = player;
  GetGame().GetGfx().SetPlayer(player);
  return this->AddEntity(player);
}

/**
 * Remove an entity from this world.
 * If the entity is the player entity, the player reference will be unset.
 * @param entity Entity to remove
 */
void
RunningState::RemoveEntity(size_t entityId) {
  auto iter = this->entities.find(entityId);
  if (iter == this->entities.end()) {
    return;
  }

  if (this->player == iter->second) {
    this->player = nullptr;
    GetGame().GetGfx().SetPlayer(nullptr);
  }
  
  delete iter->second;
  this->entities.erase(iter);
}
  

/** 
 * Find entities within an AABB.
 * The entities' AABBs have to intersesct the given AABB to be returned.
 * @param aabb AABB within which to look for entities
 * @return A vector of entities.
 */
std::vector<size_t> 
RunningState::FindEntities(const AABB &aabb) const {
  std::vector<size_t> entities;
  
  for (auto entity : this->entities) {
    if (aabb.Overlap(entity.second->GetAABB())) {
      entities.push_back(entity.first);
    }
  } 
  
  return entities;
}

std::vector<size_t> 
RunningState::FindSolidEntities(const AABB &aabb) const {
  std::vector<size_t> entities;
  
  for (auto entity : this->entities) {
    if (entity.second->IsSolid() && aabb.Overlap(entity.second->GetAABB())) {
      entities.push_back(entity.first);
    }
  } 
  
  return entities;
}

std::vector<const Entity*> 
RunningState::FindLightEntities(const Vector3 &pos, float radius) const {
  std::vector<const Entity*> entities;
  
  for (auto entity : this->entities) {
    
    if (!entity.second->GetLight().IsBlack() && (entity.second->GetPosition()-pos).GetMag() < radius) {
      entities.push_back(entity.second);
    }
  } 

  std::sort(entities.begin(), entities.end(), [&](const Entity *a, const Entity *b) -> bool {
    return (a->GetPosition()-pos).GetSquareMag() < (b->GetPosition()-pos).GetSquareMag(); 
  });
  
  return entities;
}

/**
 * Check if a cell is free of entities.
 * @param pos Cell position to check
 * @return true if no entities are inside the cell.
 */
bool 
RunningState::CheckEntities(const IVector3 &pos) {
  AABB aabb;
  aabb.extents = Vector3(0.5, 0.5, 0.5);
  aabb.center = Vector3(pos) + aabb.extents;
  
  for (auto entity : this->entities) {
    if (aabb.Overlap(entity.second->GetAABB())) return false;
  }
  
  return true;
}

Entity *
RunningState::GetEntity(size_t entityId) {
  auto iter = this->entities.find(entityId);
  if (iter == this->entities.end()) return nullptr;
  return iter->second;
}

Vector3 RunningState::MoveAABB(
  const AABB &aabb, 
  const Vector3 &targ,
  uint8_t &axis
) {
  axis = 0;
  
  Vector3 center = aabb.center;
  Vector3 dist = targ-aabb.center;

  // are we moving at all?
  if (dist.GetSquareMag() == 0.0f) return center;

  // collect all entities in range
  AABB targAABB(aabb);
  targAABB.center = targ;
  
  AABB bounds = aabb.Combine(targAABB).Grow(0.1);
  std::vector<size_t> entities = FindSolidEntities(bounds);
  if (entities.empty()) return targ;
  
  // create vertices that serve as origins for ray check. must be at most
  // one cell size apart for correct collision detection.
  std::vector<Vector3> verts;
  aabb.GetVertices(verts);
  
  Vector3 dd( dist.x>0 ? 1.0 : -1.0, dist.y>0 ? 1.0 : -1.0, dist.z>0 ? 1.0 : -1.0 );
  
  // try to move along the x axis
  if (dist.x) {
      for (size_t eid : entities) {
        Entity *ent = GetEntity(eid);
        if (!ent) continue;
        
        const AABB &entAABB = ent->GetAABB();
        
        if (entAABB.Min().z > aabb.Max().z) continue;
        if (entAABB.Max().z < aabb.Min().z) continue;
        if (entAABB.Min().y > aabb.Max().y) continue;
        if (entAABB.Max().y < aabb.Min().y) continue;
        
        float t = (aabb.center.x + dd.x * aabb.extents.x) - (entAABB.center.x - dd.x * entAABB.extents.x);
        if (t * dd.x < std::abs(dist.x)) {
          dist.x = -t - dd.x * 0.001;
          axis |= Axis::X;
        }

      }
  }
  
  // update center to new position
  center.x += dist.x;

  // try to move along the z axis
  if (dist.z) {
    for (size_t eid : entities) {
      Entity *ent = GetEntity(eid);
      if (!ent) continue;
      
      const AABB &entAABB = ent->GetAABB();
      
      if (entAABB.Min().x > aabb.Max().x) continue;
      if (entAABB.Max().x < aabb.Min().x) continue;
      if (entAABB.Min().y > aabb.Max().y) continue;
      if (entAABB.Max().y < aabb.Min().y) continue;
      
      float t = (aabb.center.z + dd.z * aabb.extents.z) - (entAABB.center.z - dd.z * entAABB.extents.z);
      if (t * dd.z < std::abs(dist.z)) {
        dist.z = -t - dd.z * 0.001;
        axis |= Axis::Z;
      }
    }
  }
  
  // update center to new position
  center.z += dist.z;

  // try to move along the y axis
  if (dist.y) {
    for (size_t eid : entities) {
      Entity *ent = GetEntity(eid);
      if (!ent) continue;
      
      const AABB &entAABB = ent->GetAABB();
      
      if (entAABB.Min().x > aabb.Max().x) continue;
      if (entAABB.Max().x < aabb.Min().x) continue;
      if (entAABB.Min().z > aabb.Max().z) continue;
      if (entAABB.Max().z < aabb.Min().z) continue;
      
      float t = (aabb.center.y + dd.y * aabb.extents.y) - (entAABB.center.y - dd.y * entAABB.extents.y);
      if (t * dd.y < std::abs(dist.y)) {
        dist.y = -t - dd.y * 0.001;
        axis |= Axis::Y;
      }
    }
  }
  
  // update center to new position
  center.y += dist.y;
  
  return center;
}

void
RunningState::Explosion(Entity &entity, const Vector3 &pos, size_t radius, float strength, float damage, Element element) {

  AABB aabb(pos, radius);
  
  size_t ownerID = entity.GetOwner();
  if (ownerID == ~0UL) ownerID = entity.GetId();
  Entity &owner = *this->entities[ownerID];

  std::vector<size_t> entIDs = this->FindEntities(aabb);
  for (size_t entID : entIDs) {
    Entity &ent  = *this->entities[entID];
    Vector3 d = ent.GetPosition() - pos;
    float dmg = damage / (1.0 + d.GetSquareMag());
    
    HealthInfo info = Stats::ExplosionAttack(owner, ent, dmg, element);
    ent.AddHealth(*this, info);
    
    try {
      dynamic_cast<Mob&>(ent).AddImpulse(d.Normalize() * dmg * 100);
    } catch(const std::bad_cast &) { }
  }
	
  IVector3 ivPos(pos);
  IVector3 ivRadius(radius, radius, radius);
  
  IVector3(radius*2, radius*2, radius*2).For( [&] (IVector3 p) {
    IVector3 worldCellPos    = p - ivRadius + ivPos;
    Vector3  worldCellCenter = Vector3(worldCellPos) + 0.5;
    Vector3  d = worldCellCenter - pos;
    float    dsqmag = d.GetSquareMag();
    
    float    chance = 1.0 / dsqmag * strength / this->world->GetCell(worldCellPos).GetInfo().breakStrength;
    if (chance > 0 && GetRandom().Chance(chance)) {
      this->GetWorld().BreakBlock(worldCellPos);
    }
  });
}

size_t
RunningState::SpawnMobInAABB(
  const std::string &type,
  const AABB &aabb,
  const Vector3 &velocity
) {
  Mob *mob = new Mob(type);
  Vector3 s = aabb.extents - mob->GetAABB().extents;
  Vector3 p = GetRandom().Vector() * s + aabb.center;
  mob->SetPosition(p);
  mob->AddVelocity(velocity);
  return AddEntity(mob);
}

void 
RunningState::Save() {
  PROFILE();
  Log("Saving...\n");
  this->saving = true;
  
  Serializer serGame("GAME");
  serGame << GetGame();
  serGame << self;
  
  FILE *f = createUserFile("game");
  if (f) {
    serGame.WriteToFile(f);
    fclose(f); 
    f = nullptr;
  }
  
  SaveLevel();
  
  this->saving = false;
}

void 
RunningState::SaveLevel() {
  Serializer ser("LEVL");
  ser << *world;
  ser << this->player->GetId();
  ser << this->entities;
  
  FILE *f = createUserFile("level." + ToString(level));
  
  //std::thread([=](){
    if (f) {
      ser.WriteToFile(f);
      fclose(f);
    }
  //}).detach();
}

void 
RunningState::LoadLevel() {
  FILE *f = openUserFile("level." + ToString(level));
  
  Deserializer deser;
  deser.LoadFromFile(f, "LEVL");
  fclose(f);
  
  delete world;
  world = new World(*this, deser);

  size_t playerId;
  deser >> playerId;
  deser >> this->entities;
  
  this->player = dynamic_cast<Player*>(this->entities[playerId]);
  GetGame().GetGfx().SetPlayer(this->player);
  
  for (auto &e:this->entities) {
    e.second->Continue(*this, e.first);
  }
  
}

Serializer &operator << (Serializer &ser, const RunningState &state) {
  ser << state.level;
  ser << state.nextEntityId;
  return ser;
}

void 
RunningState::Load() {
  PROFILE();
  Log("Loading...\n");
  
  Deserializer deser;
  FILE *f = openUserFile("game");
  deser.LoadFromFile(f, "GAME");

  Log("game %08x\n", deser.GetPos());
  deser >> GetGame();
  
  Log("runningstate %08x\n", deser.GetPos());
  deser >> self;
  
  LoadLevel();
}

Deserializer &operator >> (Deserializer &deser, RunningState &state) {
  deser >> state.level; 
  Log("%u\n", state.level);
  deser >> state.nextEntityId;
  Log("%u\n", state.nextEntityId);
  return deser;
}
