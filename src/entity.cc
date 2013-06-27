#include <GL/glfw.h>
#include <cstring>

#include "entity.h"
#include "world.h"
#include "cell.h"
#include "util.h"
#include "item.h"
#include "itementity.h"
#include "random.h"
#include "game.h"
#include "gfx.h"

static std::map<std::string, EntityProperties> allEntities;
EntityProperties defaultEntity;

const EntityProperties *getEntity(const std::string &name) {
  if (allEntities.find(name) == allEntities.end()) {
    std::cerr << "entity " << name << " not found" << std::endl;
    return &defaultEntity;
  }
  return &allEntities[name];
}

EntityProperties::EntityProperties() {
}

EntityProperties::EntityProperties(FILE *f) {
  char line[256];
  
  while(fgets(line, 256, f) && !feof(f)) {
    if (line[0] == '#') continue;
    
    std::vector<std::string> tokens;
    char *p = line;
    char *q;
    do {
      q = strchr(p, ' ');
      if (!q) q = strchr(p, '\r');
      if (!q) q = strchr(p, '\n');
      if (q) *q = 0;
      tokens.push_back(p);
      if (q) { p = q+1; }
    } while(q);
    if (tokens.size() == 0) continue;

    if (tokens[0] == "tex") {
      this->sprite.texture = loadTexture("entities/texture/"+tokens[1]);
    } else if (tokens[0] == "frames") {
      this->sprite.totalFrames = std::atoi(tokens[1].c_str());
    } else if (tokens[0] == "anim") {
      this->sprite.animations.push_back(Animation(std::atoi(tokens[1].c_str()), std::atoi(tokens[2].c_str()), std::atof(tokens[3].c_str())));
    } else if (tokens[0] == "vert") {
      this->sprite.vertical = true;
    } else if (tokens[0] == "step") {
      this->stepHeight = std::atof(tokens[1].c_str());
    } else if (tokens[0] == "mass") {
      this->mass = std::atof(tokens[1].c_str());
    } else if (tokens[0] == "move") {
      this->moveInterval = std::atof(tokens[1].c_str());
    } else if (tokens[0] == "speed") {
      this->maxSpeed = std::atof(tokens[1].c_str());
    } else if (tokens[0] == "health") {
      this->maxHealth = std::atoi(tokens[1].c_str());
    } else if (tokens[0] == "extents") {
      this->extents = Vector3( std::atof(tokens[1].c_str()), std::atof(tokens[2].c_str()), std::atof(tokens[3].c_str()) );
      this->sprite.width = this->extents.x;
      this->sprite.height = this->extents.y;
    } else if (tokens[0] == "size") {
      this->sprite.width = std::atof(tokens[1].c_str());
      this->sprite.height = std::atof(tokens[2].c_str());
    } else if (tokens[0] == "inventory") {
      this->items[tokens[2]] = std::atof(tokens[1].c_str());
    } else if (tokens[0] == "cell") {
      this->cellEnter = tokens[1];
      this->cellLeave = tokens[2];
    } else if (tokens[0] != "") {
      std::cerr << "ignoring '" << tokens[0] << "'" << std::endl;
    }
  }
}

void
LoadEntities() {
  std::vector<std::string> assets = findAssets("entities");
  for (const std::string &name : assets) {
    FILE *f = openAsset("entities/"+name);
    if (f) {
      std::cerr << "loading entity " << name << std::endl;
      allEntities[name] = EntityProperties(f);
      fclose(f);
    }
  }
}

Entity::Entity(const std::string &type) {
  this->removable = false;
  
  this->properties = getEntity(type);
  this->sprite = this->properties->sprite;

  this->aabb.extents = properties->extents;
  this->inventory.resize(32, nullptr);
  
  this->health = properties->maxHealth;
  this->lastCell = nullptr;
}

Entity::~Entity() {
}

void
Entity::Start() {
  std::shared_ptr<World> world = Game::Instance->GetWorld();
  
  // fill inventory with random crap
  for (auto item : this->properties->items) {
    if (world->GetRandom().Chance(item.second)) {
      std::cerr << item.first << std::endl;
      this->AddToInventory(std::shared_ptr<Item>(new Item(item.first)));
    }
  }
  
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

  for (Vector3 v : verts) {
    if (world->IsPointSolid(aabb.center + v)) {
      aabb.center = aabb.center - v;
    }
  }
}

void 
Entity::Update() {
  this->sprite.Update(Game::Instance->GetDeltaT());
  
  IVector3 cellPos(aabb.center.x, aabb.center.y, aabb.center.z);
  Cell *cell = &Game::Instance->GetWorld()->GetCell(cellPos);
  if (cell != this->lastCell) {
    if (this->lastCell && this->properties->cellLeave != "") {
      Game::Instance->GetWorld()->SetCell(this->lastCell->GetPosition(), Cell(this->properties->cellLeave));
    }
    if (this->properties->cellEnter != "") {
      Game::Instance->GetWorld()->SetCell(cellPos, Cell(this->properties->cellEnter));
    }
    this->lastCell = cell;
  }
  
  for (size_t i=0; i<this->inventory.size(); i++) {
    if (this->inventory[i]) {
      this->inventory[i]->Update();
      if (this->inventory[i]->IsRemovable()) {
        this->inventory[i] = nullptr;
      }
    }
  }
  
  this->light = Game::Instance->GetWorld()->GetLight(cellPos).Saturate();
  
  this->smoothPosition = this->smoothPosition + (aabb.center - this->smoothPosition) * Game::Instance->GetDeltaT() * 10.0f;
}

void
Entity::Draw() const {
  glColor3ub(light.r, light.g, light.b);
  Gfx::Instance->DrawSprite(this->sprite, this->aabb.center);
  
  if (glfwGetKey(GLFW_KEY_F2)) {
    glColor3f(0.25,0.25,0.25);
    this->DrawBoundingBox();
  }
}

void
Entity::DrawBoundingBox() const {
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE);
  
  glDepthMask(GL_FALSE);
    
  glBindTexture(GL_TEXTURE_2D, 0);
  
  Gfx::Instance->DrawAABB(this->aabb);
  
  glDepthMask(GL_TRUE);
  glDisable(GL_BLEND);
}

void
Entity::AddHealth(int points) {
  if (health == 0 || this->properties->maxHealth == 0) return;
  
  health += points;
  if (health <= 0) {
    health = 0;
    Die();
  }
}

void
Entity::Die() {
  this->removable = true;
  
  if (this->lastCell && this->properties->cellLeave != "") {
    Game::Instance->GetWorld()->SetCell(this->lastCell->GetPosition(), Cell(this->properties->cellLeave));
  }
  
  // drop inventory
  for (auto item : this->inventory) {
    if (item) {
      Entity *entity = new ItemEntity(item);
      entity->SetPosition(this->aabb.center);
      Game::Instance->AddEntity(entity);
    }
  }
}

bool
Entity::AddToInventory(const std::shared_ptr<Item> &item) {
  for (size_t i=(size_t)InventorySlot::Backpack; i<this->inventory.size(); i++) {
    if (!this->inventory[i]) {
      this->inventory[i] = item;
      item->SetEquipped(false);
      return true;
    }
  }
  return false;
}

bool
Entity::AddToInventory(const std::shared_ptr<Item> &item, InventorySlot slot) {
  size_t i = (size_t)slot;
  if (!this->inventory[i]) {
    if (i>=(size_t)InventorySlot::Backpack ||(item->GetProperties()->equippable & 1<<i)) {
      this->Equip(item, slot);
      return true;
    } else {
      return this->AddToInventory(item);
    }
  }

  std::shared_ptr<Item> combo;
  combo = item->Combine(this->inventory[i]);
  if (!combo) combo = this->inventory[i]->Combine(item);
  if (combo) {
    this->Equip(combo, slot);
    return true;
  }

  if (this->AddToInventory(this->inventory[i])) {
    this->inventory[i] = nullptr;
    this->Equip(item, slot);
    return true;
  }
  return false;
}
 
void 
Entity::Equip(const std::shared_ptr<Item> &item, InventorySlot slot) {
  bool equip = (size_t)slot < (size_t)InventorySlot::Backpack;
  if (this->inventory[(size_t)slot] && item) {
    this->inventory[(size_t)slot]->SetEquipped(false);
    this->AddToInventory(this->inventory[(size_t)slot]);
  }
  this->inventory[(size_t)slot] = item;
  if (item) item->SetEquipped(equip);
}

