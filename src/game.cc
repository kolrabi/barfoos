#include "gfx.h"
#include "game.h"
#include "world.h"
#include "player.h"
#include "worldedit.h"
#include "feature.h"
#include "random.h"
#include "inventorygui.h"
#include "random.h"
#include "item.h"
#include "input.h"

Game::Game(const std::string &seed, size_t level, const Point &screenSize) 
: input(new Input()), 
  gfx(new Gfx(Point(1920, 32), screenSize, false)),
  level(level),
  seed(seed), 
  random(seed, level)
  {

  this->player = nullptr;
  this->nextEntityId = 0;
  this->startT = 0.0;
  this->lastT = 0.0;
  this->deltaT = 0.0;
  this->nextThinkT = 0.0;
  this->frame = 0;
  this->lastFPST = 0;
  this->fps = 0;
  this->showInventory = false;
  
  this->handlerId = this->input->AddHandler( [this](const InputEvent &event){ this->HandleEvent(event); } );
  this->isInit = false;
}

Game::~Game() {
  if (isInit) this->Deinit();
  
  this->input->RemoveHandler(this->handlerId);
  
  delete gfx;
  delete input;
}

bool 
Game::Init() {
  std::cerr << "initializing game" << std::endl;
  if (!this->gfx->Init(*this)) return false;
  
  LoadCells();
  LoadFeatures();
  LoadEntities();
  LoadItems();
  
  this->nextEntityId = 0;
  this->showInventory = false;
  this->lastT = 0;
  this->deltaT = 0;
  this->nextThinkT = 0;
  this->frame = 0;
  this->lastFPST = 0;
  this->fps = 0;

  this->BuildWorld();

  this->startT = this->gfx->GetTime();

  this->isInit = true;
  return true;
}

void Game::Deinit() {
  for (auto entity : this->entities) {
    delete entity.second;
  }
}

bool Game::Frame() {
  PROFILE();
  
  if (!this->gfx->Swap()) return false;
  
  // render game
  this->Render();
  
  // update game (at most 0.1s at a time)
  float t = this->gfx->GetTime() - this->startT;
  while(t - this->lastT > 0.1) {
    this->lastT += 0.1;
    this->Update(lastT, 0.1);
    this->input->Update();
  }
  this->Update(t, t - this->lastT);
  this->lastT = t;
  
  this->gfx->Update(*this);
  
  this->frame ++;
  if (t - this->lastFPST >= 1.0) {
    this->fps = this->frame / (t-this->lastFPST);
    this->frame = 0;
    this->lastFPST += 1.0;
  }
  return true;
}

void
Game::Render() const {
  PROFILE();
  
  std::vector<IColor> lightColors;
  std::vector<Vector3> lightPositions;
  std::vector<const Entity*> lightEntities = FindLightEntities(player->GetPosition(), 32);
  if (lightEntities.size() > 8) lightEntities.resize(8);
  for (auto e : lightEntities) {
    lightColors.push_back(e->GetLight());
    lightPositions.push_back(e->GetPosition());
  }
  
  this->gfx->ClearColor(IColor(30, 30, 20));
  this->gfx->ClearDepth(1.0);
  this->gfx->Viewport(Rect());

  // draw world first
  this->gfx->SetFog(0.051, 0.05, IColor(20,20,20));
  this->gfx->SetLights(lightPositions, lightColors);
  
  player->View(*this->gfx);
  world->Draw(*this->gfx);
  
  // draw all entities
  for (auto entity : this->entities) {
    entity.second->Draw(*this->gfx);
  }
  
  this->gfx->ClearDepth(1.0);
  player->DrawWeapons(*this->gfx);

  // next draw gui stuff
  this->gfx->GetView().GUI();
  
  if (this->activeGui) {
    this->activeGui->Draw(*this->gfx, Point());
  }
  player->DrawGUI(*this->gfx);
}

float Game::GetThinkFraction() const {
  float lastThinkT = this->nextThinkT - Entity::ThinkInterval;
  return (this->lastT - lastThinkT) / Entity::ThinkInterval;
}

void 
Game::Update(float t, float deltaT) {
  PROFILE();
  
  this->lastT = t;
  this->deltaT = deltaT;
  
  world->Update(*this);

  if (this->input->IsKeyActive(InputKey::Inventory)) {
    if (!this->showInventory) {
      if (this->activeGui) {
        this->activeGui->OnHide();
        this->gfx->DecGuiCount();
      }
      this->activeGui = this->inventoryGui;
      this->inventoryGui->SetForward(this->player->GetAngles().EulerToVector());
      this->activeGui->OnShow();
      this->gfx->IncGuiCount();
    }
    this->showInventory = true;
  } else {
    if (this->showInventory) {
      if (this->activeGui) {
        this->activeGui->OnHide();
        this->gfx->DecGuiCount();
      }
      this->activeGui = nullptr;
    }
    this->showInventory = false;
  }
  if (this->activeGui) this->activeGui->Update(*this);
  
  // handle collision between entities
  for (auto entity1 : this->entities) {
    for (auto entity2 : this->entities) {
      if (entity1.first == entity2.first) continue;
      
      if (entity1.second->GetOwner() == entity2.first && entity1.second->GetProperties()->nocollideOwner) continue;
      if (entity2.second->GetOwner() == entity1.first && entity2.second->GetProperties()->nocollideOwner) continue;
      
      if (entity1.second->GetProperties()->nocollideEntity) continue;
      if (entity2.second->GetProperties()->nocollideEntity) continue;
      
      if (entity1.second->GetAABB().Overlap(entity2.second->GetAABB())) {
        entity1.second->OnCollide(*this, *entity2.second);
        entity2.second->OnCollide(*this, *entity1.second);
      }
    }
  }

  // update all entities
  {
  PROFILE_NAMED("update"); 
  size_t count = 0;
  for (auto entity : this->entities) {
    if (entity.second) { entity.second->Update(*this); count++; }
  }
  std::cerr << count << "/" << this->entities.size() << std::endl;
  }
  
  {
  PROFILE_NAMED("think"); 
  while(nextThinkT < t) {
    nextThinkT += Entity::ThinkInterval;
    for (auto entity : this->entities) {
      if (entity.second) entity.second->Think(*this);
    }
  }
  }

  // remove removable entities
  {
  PROFILE_NAMED("remove"); 
  size_t count = 0;
  auto entityIter = this->entities.begin();
  while(entityIter != this->entities.end()) {
    if (!entityIter->second || entityIter->second->IsRemovable()) {
      delete entityIter->second;
      entityIter = this->entities.erase(entityIter);
      count++;
    } else {
      entityIter++;
    }
  }
  std::cerr << count << "/" << this->entities.size() << std::endl;
  }
}

void 
Game::BuildWorld() { 
  PROFILE();

  random.Seed(seed, level); 
  this->world = std::shared_ptr<World>(new World(*this, IVector3(64, 64, 64)));
  this->world->Build(*this);

  Player *player = new Player();
  player->SetPosition(IVector3(32,32,32));
  player->SetSpawnPos(IVector3(32,32,32));
  this->AddPlayer(player);
  
  Entity *entity = new Entity("box");
  entity->SetPosition(IVector3(32,24,32));
  this->AddEntity(entity);
}

void Game::HandleEvent(const InputEvent &event) {
  if (this->activeGui) {
    this->activeGui->HandleEvent(event);
  } else {
    this->player->HandleEvent(event);
  }
}

/**
 * Add an entity to this game.
 * @param entity Entity to add
 */
size_t
Game::AddEntity(Entity *entity) {
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
Game::AddPlayer(Player *player) {
  this->player = player;
  this->inventoryGui = std::shared_ptr<InventoryGui>(new InventoryGui(*this, *player));
  return this->AddEntity(player);
}

/**
 * Remove an entity from this world.
 * If the entity is the player entity, the player reference will be unset.
 * @param entity Entity to remove
 */
void
Game::RemoveEntity(size_t entityId) {
  auto iter = this->entities.find(entityId);
  if (iter == this->entities.end()) {
    return;
  }

  if (this->player == iter->second) {
    this->player = nullptr;
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
Game::FindEntities(const AABB &aabb) const {
  std::vector<size_t> entities;
  
  for (auto entity : this->entities) {
    if (aabb.Overlap(entity.second->GetAABB())) {
      entities.push_back(entity.first);
    }
  } 
  
  return entities;
}

std::vector<size_t> 
Game::FindSolidEntities(const AABB &aabb) const {
  std::vector<size_t> entities;
  
  for (auto entity : this->entities) {
    if (entity.second->IsSolid() && aabb.Overlap(entity.second->GetAABB())) {
      entities.push_back(entity.first);
    }
  } 
  
  return entities;
}

std::vector<const Entity*> 
Game::FindLightEntities(const Vector3 &pos, float radius) const {
  std::vector<const Entity*> entities;
  
  for (auto entity : this->entities) {
    
    if (!entity.second->GetLight().IsBlack() && (entity.second->GetPosition()-pos).GetMag() < radius) {
      entities.push_back(entity.second);
    }
  } 
  
  return entities;
}

/**
 * Check if a cell is free of entities.
 * @param pos Cell position to check
 * @return true if no entities are inside the cell.
 */
bool 
Game::CheckEntities(const IVector3 &pos) {
  AABB aabb;
  aabb.extents = Vector3(0.5, 0.5, 0.5);
  aabb.center = Vector3(pos) + aabb.extents;
  
  for (auto entity : this->entities) {
    if (aabb.Overlap(entity.second->GetAABB())) return false;
  }
  
  return true;
}

temp_ptr<Entity> 
Game::GetEntity(size_t entityId) {
  auto iter = this->entities.find(entityId);
  if (iter == this->entities.end()) {
    return temp_ptr<Entity>(nullptr);
  }
  return temp_ptr<Entity>(iter->second);
}

Vector3 Game::MoveAABB(
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
  for (const Vector3 &v : verts) {
    for (size_t eid : entities) {
      temp_ptr<Entity> ent = GetEntity(eid);
      if (!ent) continue;
      
      float t = INFINITY;
      Vector3 p;
      
      if (ent->GetAABB().Ray(center + v, Vector3(dd.x,0,0), t, p) && t < std::abs(dist.x)+0.001 && t >= 0) {
        dist.x = t - dd.x * 0.001;
        axis |= Axis::X;
      }
    }
  }
  }
  
  // update center to new position
  center.x += dist.x;

  // try to move along the z axis
  if (dist.z) {
  for (const Vector3 &v : verts) {
    for (size_t eid : entities) {
      temp_ptr<Entity> ent = GetEntity(eid);
      if (!ent) continue;
      
      float t = INFINITY;
      Vector3 p;
      
      if (ent->GetAABB().Ray(center + v, Vector3(0,0,dd.z), t, p) && t < std::abs(dist.z)+0.001 && t >= 0) {
        dist.z = t - dd.z * 0.001;
        axis |= Axis::Z;
      }
    }
  }
  }
  
  // update center to new position
  center.z += dist.z;

  // try to move along the y axis
  if (dist.y) {
  for (const Vector3 &v : verts) {
    for (size_t eid : entities) {
      temp_ptr<Entity> ent = GetEntity(eid);
      if (!ent) continue;
      
      float t = INFINITY;
      Vector3 p;
      
      if (ent->GetAABB().Ray(center + v, Vector3(0,dd.y,0), t, p) && t < std::abs(dist.y)+0.001 && t >= 0) {
        dist.y = t - dd.y * 0.001;
        axis |= Axis::Y;
      }
    }
  }
  }
  
  // update center to new position
  center.y += dist.y;
  
  return center;
}

void
Game::Explosion(const IVector3 &pos, const IVector3 &size, float strength) {
  Vector3 v(pos);
  Vector3 vs(size);
  
  IVector3(size.x*2+1, size.y*2+1, size.z*2+1).For( [&] (IVector3 p) {
    IVector3 pp = pos - size + p;
    Vector3 vpp(pp);
    float d = (vpp-v).GetSquareMag()/4;
    float prob = (vs.GetMag()/2-d) * strength / this->world->GetCell(pp).GetInfo().breakStrength;
    if (prob > 0 && random.Chance(prob)) {
      this->GetWorld().BreakBlock(*this, pp);
    }
  });
}
