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
#include "effect.h"

#include <algorithm>

Game::Game(const Point &screenSize) : 
  isInit        (false),
  input         (new Input()), 
  gfx           (new Gfx(Point(1920, 32), screenSize, false)),
  world         (nullptr),
  handlerId     (this->input->AddHandler( [this](const InputEvent &event){ this->HandleEvent(event); } )),
  entities      (),
  player        (nullptr),
  nextEntityId  (1),
  inventoryGui  (nullptr),
  activeGui     (nullptr),
  startT        (0.0),
  lastT         (0.0),
  deltaT        (0.0),
  frame         (0),
  lastFPST      (0.0),
  fps           (0.0),
  seed          (""), 
  random        (""),
  showInventory (false)
{
}

Game::~Game() {
  if (this->isInit) this->Deinit();
  
  this->input->RemoveHandler(this->handlerId);
  
  delete gfx;
  delete input;
}

bool 
Game::Init() {
  if (this->isInit) return true;
  
  Log("Initializing game\n");
  if (!this->gfx->Init(*this)) return false;
  
  this->nextEntityId = 0;
  this->showInventory = false;
  this->lastT = 0;
  this->deltaT = 0;
  this->frame = 0;
  this->lastFPST = 0;
  this->fps = 0;

  this->world = nullptr;

  this->isInit = true;
  return true;
}

void
Game::NewGame(const std::string &seed) {
  this->level = 0;
  this->random.Seed(seed, 0);

  std::string scrolls = loadAssetAsString("text/scrolls");
  Log("%s\n", scrolls.c_str());
  scrollMarkov.add(0, scrolls.begin(), scrolls.end());
  
  LoadCells();
  LoadFeatures();
  LoadEntities();
  LoadEffects();
  LoadItems(*this);
  
  this->BuildWorld();
  this->startT = this->gfx->GetTime();
}

void Game::Deinit() {
  for (auto entity : this->entities) {
    delete entity.second;
  }
}

bool Game::Frame() {
  PROFILE();
  
  if (!this->gfx->Swap()) return false;
  
  // update game (at most 0.1s at a time)
  float t = this->gfx->GetTime() - this->startT;
  while(t - this->lastT > 0.1) {
    this->lastT += 0.1;
    this->Update(lastT, 0.1);
    this->input->Update();
  }
  this->Update(t, t - this->lastT);
  this->lastT = t;
  
  // render game
  this->Render();

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
  std::vector<const Entity*> lightEntities = FindLightEntities(player->GetSmoothPosition(), 32);
  for (auto e : lightEntities) {
    lightColors.push_back(e->GetLight());
    lightPositions.push_back(e->GetSmoothEyePosition());
  }
  
  this->gfx->ClearColor(IColor(30, 30, 20));
  this->gfx->ClearDepth(1.0);
  this->gfx->Viewport(Rect());

  // draw world first
  this->gfx->SetFog(0.05, IColor(64,64,64));
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
  
  player->DrawGUI(*this->gfx);
  
  if (this->activeGui) {
    this->activeGui->Draw(*this->gfx, Point());
    this->activeGui->DrawTooltip(*this->gfx, Point());
  }
}

void 
Game::Update(float t, float deltaT) {
  PROFILE();

  this->lastT = t;
  this->deltaT = deltaT;
  
  // show or hide inventory
  if (this->input->IsKeyActive(InputKey::Inventory)) {
    if (!this->showInventory) {
      if (this->activeGui) {
        this->activeGui->OnHide();
        this->gfx->DecGuiCount();
      }
      this->activeGui = this->inventoryGui;
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

    for (size_t j=i+1; j<entityIds.size(); j++) {
      size_t e2 = entityIds[j];
      if (entities[e2]->GetProperties()->nocollideEntity) continue;
      
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
        delete entityIter->second;
        entityIter = this->entities.erase(entityIter);
      } else {
        entityIter++;
      }
    }
  }
  
  this->input->Update();
}

void 
Game::BuildWorld() { 
  PROFILE();

  delete this->world;
  this->world = new World(*this, IVector3(64, 64, 64));
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
  this->gfx->SetPlayer(player);
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
    this->gfx->SetPlayer(nullptr);
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
Game::CheckEntities(const IVector3 &pos) {
  AABB aabb;
  aabb.extents = Vector3(0.5, 0.5, 0.5);
  aabb.center = Vector3(pos) + aabb.extents;
  
  for (auto entity : this->entities) {
    if (aabb.Overlap(entity.second->GetAABB())) return false;
  }
  
  return true;
}

Entity *
Game::GetEntity(size_t entityId) {
  auto iter = this->entities.find(entityId);
  if (iter == this->entities.end()) return nullptr;
  return iter->second;
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
Game::Explosion(Entity &entity, const Vector3 &pos, size_t radius, float strength, float damage, Element element) {

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
    if (chance > 0 && random.Chance(chance)) {
      this->GetWorld().BreakBlock(*this, worldCellPos);
    }
  });
}

std::string 
Game::GetScrollName() {
  if (scrollMarkov.size() == 0) return "ERROR";

  std::string name;
  char c = 0;
  size_t l = 0;
  while(true) {
    c = scrollMarkov[c].select(this->random.Float01());
    if (c == '\n') c = ' ';
    if (c == ' ' && l > 10) break;
    name += c;
    l++;
  }
  Log("Created scroll name '%s'\n", name.c_str());
  return name;
}

