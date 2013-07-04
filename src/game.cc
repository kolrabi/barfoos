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
  seed(seed), 
  random(seed, level),
  level(level)  {
 
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

  this->BuildWorld(level);

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
  return true;
}

void
Game::Render() const {
  this->gfx->ClearColor(IColor(30, 30, 20));
  this->gfx->ClearDepth(1.0);
  this->gfx->Viewport(Rect());

  // draw world first
  this->gfx->SetFog(0.051, 0, IColor(20,20,20));
  
  player->View(*this->gfx);
  world->Draw(*this->gfx);
  
  // draw all entities
  for (auto entity : this->entities) {
    entity.second->Draw(*this->gfx);
  }
  
  this->gfx->ClearDepth(1.0);
  player->DrawWeapons(*this->gfx);

  // next draw gui stuff
  this->gfx->ViewGUI();
  
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
      if (entity1.second->GetProperties()->nocollide) continue;
      if (entity2.second->GetProperties()->nocollide) continue;
      if (entity1.second->GetAABB().Overlap(entity2.second->GetAABB())) {
        entity1.second->OnCollide(*this, *entity2.second);
        entity2.second->OnCollide(*this, *entity1.second);
      }
    }
  }

  // update all entities
  for (auto entity : this->entities) {
    if (entity.second) entity.second->Update(*this);
  }
  
  while(nextThinkT < t) {
    nextThinkT += Entity::ThinkInterval;
    for (auto entity : this->entities) {
      if (entity.second) entity.second->Think(*this);
    }
  }

  // remove removable entities
  auto entityIter = this->entities.begin();
  while(entityIter != this->entities.end()) {
    if (entityIter->second->IsRemovable()) {
      delete entityIter->second;
      entityIter = this->entities.erase(entityIter);
    } else {
      entityIter++;
    }
  }
}

void 
Game::BuildWorld(size_t level) { 
  random.Seed(seed, level); 
  this->world = std::shared_ptr<World>(new World(IVector3(64, 64, 64), level, random));
  this->world->Build(*this);

  Player *player = new Player();
  player->SetPosition(IVector3(32,32,32));
  player->SetSpawnPos(IVector3(32,32,32));
  this->AddPlayer(player);

  Entity *ent = new Entity("box");
  ent->SetPosition(IVector3(32,28,34));
  this->AddEntity(ent);
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
Game::FindEntities(const AABB &aabb) {
  std::vector<size_t> entities;
  
  for (auto entity : this->entities) {
    if (aabb.Overlap(entity.second->GetAABB())) {
      entities.push_back(entity.first);
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

