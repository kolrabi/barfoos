#include <GL/glfw.h>
#include "gfx.h"

#include "game.h"
#include "world.h"
#include "player.h"
#include "worldedit.h"
#include "feature.h"
#include "random.h"
#include "inventorygui.h"

Game *Game::Instance = nullptr;

Game::Game(const std::string &seed, size_t level) : seed(seed), random(seed, level) {
  delete Game::Instance;
  Game::Instance = this;

  LoadCells();
  LoadFeatures();
  LoadEntities();
  LoadItems();
  
  this->nextEntityId = 0;
  this->showInventory = false;
  this->lastT = 0;
  this->deltaT = 0;

  this->BuildWorld(level);
}

Game::~Game() {
  for (auto entity : this->entities) {
    delete entity.second;
  }
  Game::Instance = nullptr;
}

void
Game::Render() const {
  glClearColor(0.3,0.3,0.2, 1.0);
  glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);
  Gfx::Instance->Viewport(Rect());

  // draw world first
  if (glfwGetKey('Q')) {
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  } else {
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  }
  glFogi(GL_FOG_MODE, GL_EXP2);
  glFogf(GL_FOG_DENSITY, 0.051f);
  
  player->View();
  world->Draw();
  
  // draw all entities
  for (auto entity : this->entities) {
    entity.second->Draw();
  }
  
  glClear(GL_DEPTH_BUFFER_BIT);
  player->DrawWeapons();
/*
  // next draw mini map
  const Point &ssize = Gfx::Instance->GetScreenSize();
  if (glfwGetKey('M')) {
    Gfx::Instance->Viewport(Rect(Point(ssize.x/2 - ssize.y/2, 0), Point(ssize.y, ssize.y)));
  } else {
    Gfx::Instance->Viewport(Rect(Point(ssize.x - ssize.x/8, ssize.y - ssize.x/8), Point(ssize.x/8, ssize.x/8)));
  }
  glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);

//  glDisable(GL_FOG);
  glFogi(GL_FOG_MODE, GL_LINEAR);
  player->MapView();
  world->DrawMap();
//  glEnable(GL_FOG);

  Gfx::Instance->Viewport(Rect());
*/
  // next draw debug stuff
  //glDisable(GL_DEPTH_TEST);
  glColor3ub(255,255,0);
  player->DrawGUI();
  
  if (this->activeGui) {
    Gfx::Instance->ViewGUI();
    this->activeGui->Draw(Point());
  }
}

void 
Game::Update(float t, float deltaT) {
  this->lastT = t;
  this->deltaT = deltaT;
  
  world->Update(t);

  if (glfwGetKey(GLFW_KEY_TAB)) {
    if (!this->showInventory) {
      if (this->activeGui) {
        this->activeGui->OnHide();
        Gfx::Instance->DecGuiCount();
      }
      this->activeGui = this->inventoryGui;
      this->inventoryGui->SetForward(this->player->GetAngles().EulerToVector());
      this->activeGui->OnShow();
      Gfx::Instance->IncGuiCount();
    }
    this->showInventory = true;
  } else {
    if (this->showInventory) {
      if (this->activeGui) {
        this->activeGui->OnHide();
        Gfx::Instance->DecGuiCount();
      }
      this->activeGui = nullptr;
    }
    this->showInventory = false;
  }

  if (this->activeGui) this->activeGui->Update(t);
  
  // handle collision between entities
  for (auto entity1 : this->entities) {
    for (auto entity2 : this->entities) {
      if (entity1.first == entity2.first) continue;
      if (entity1.second->GetAABB().Overlap(entity2.second->GetAABB())) {
        entity1.second->OnCollide(*entity2.second);
        entity2.second->OnCollide(*entity1.second);
      }
    }
  }

  // update all entities
  for (auto entity : this->entities) {
    if (entity.second) entity.second->Update();
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
  this->world->Build();

  Player *player = new Player();
  player->SetPosition(IVector3(32,32,32));
  player->SetSpawnPos(IVector3(32,32,32));
  size_t playerId = this->AddPlayer(player);

  this->inventoryGui = std::shared_ptr<InventoryGui>(new InventoryGui(playerId));
}

void Game::OnMouseMove(const Point &pos) {
  if (this->activeGui) {
    this->activeGui->OnMouseMove(pos);
  }
}

void Game::OnMouseClick(const Point &pos, int button, bool down) {
  if (this->activeGui) {
    this->activeGui->OnMouseClick(pos, button, down);
  } else {
    this->player->OnMouseClick(pos, button, down);
  }
}

void Game::OnMouseDelta(const Point &delta) {
  this->player->OnMouseDelta(delta);
}

void Game::OnKey(int key, bool down) {
  if (down && key == GLFW_KEY_F12) {
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
  entity->Start();
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

