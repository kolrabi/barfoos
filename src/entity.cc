#include <GL/glfw.h>
#include <cstring>

#include "entity.h"
#include "world.h"
#include "cell.h"
#include "util.h"
#include "item.h"
#include "itementity.h"
#include "random.h"

static std::map<std::string, EntityProperties *> allEntities;
EntityProperties defaultEntity;

const EntityProperties *getEntity(const std::string &name) {
  if (allEntities.find(name) == allEntities.end()) {
    std::cerr << "entity " << name << " not found" << std::endl;
    return &defaultEntity;
  }
  return allEntities[name];
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
      this->texture = loadTexture("entities/texture/"+tokens[1]);
    } else if (tokens[0] == "frames") {
      this->frames = std::atoi(tokens[1].c_str());
    } else if (tokens[0] == "anim") {
      this->anims.push_back(Animation(std::atoi(tokens[1].c_str()), std::atoi(tokens[2].c_str()), std::atof(tokens[3].c_str())));
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
      this->w = this->extents.x;
      this->h = this->extents.y;
    } else if (tokens[0] == "size") {
      this->w = std::atof(tokens[1].c_str());
      this->h = std::atof(tokens[2].c_str());
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
      allEntities[name] = new EntityProperties(f);
      fclose(f);
    }
  }
}

Entity::Entity(const std::string &type) {
  this->world = nullptr;
  this->removable = false;
  
  this->properties = getEntity(type);
  
  this->lastT = 0;
  this->deltaT = 0;
  
  this->frame = 0;
  this->animation = 0;
  
  this->aabb.extents = properties->extents;
  this->inventory.resize(32, nullptr);
  
  this->health = properties->maxHealth;
  this->lastCell = nullptr;
}

Entity::~Entity() {
}

void
Entity::SetWorld(World *world) {
  if (!this->world && world) {
    for (auto item : this->properties->items) {
      if (world->GetRandom().Chance(item.second)) {
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
  
  this->world = world;
}

void 
Entity::Update(float t) {
  if (!this->world) return;
  
  // update time
  if (lastT == 0) lastT = t;
  deltaT = t - lastT;
  
  if (this->properties->anims.size() > 0) {
    const Animation &a = this->properties->anims[animation];
    frame += a.fps * deltaT;
    if (frame >= a.frameCount+a.firstFrame) {
      animation = 0;
      frame = frame - (int)frame + this->properties->anims[0].firstFrame;
    }
  }
  
  IVector3 cellPos(aabb.center.x, aabb.center.y, aabb.center.z);
  Cell *cell = &this->world->GetCell(cellPos);
  if (cell != this->lastCell) {
    if (this->lastCell && this->properties->cellLeave != "") {
      this->world->SetCell(this->lastCell->GetPosition(), Cell(this->properties->cellLeave));
    }
    if (this->properties->cellEnter != "") {
      this->world->SetCell(cellPos, Cell(this->properties->cellEnter));
    }
    this->lastCell = cell;
  }
  
  for (size_t i=0; i<this->inventory.size(); i++) {
    if (this->inventory[i]) {
      this->inventory[i]->Update(t);
      if (this->inventory[i]->IsRemovable()) {
        this->inventory[i] = nullptr;
      }
    }
  }
  
  this->light = this->world->GetLight(cellPos).Saturate();
  
  if (deltaT > 1.0/10.0)
    smoothPosition = aabb.center;
  else 
    smoothPosition = smoothPosition + (aabb.center - smoothPosition) * deltaT * 10.0f;
    
  lastT = t;
}

void
Entity::Draw() {
  if (this->properties->texture != 0) {
    float u = 0.0;
    float uw = 1.0;
    if (this->properties->frames) {
      int f = ((int)this->frame) % this->properties->frames;
      uw = 1.0/this->properties->frames;
      u = f*uw;
    }
    
    glColor3ub(light.r, light.g, light.b);
    drawBillboard(aabb.center, this->properties->w/2.0, this->properties->h/2.0, this->properties->texture, u, uw, -(this->properties->originX-0.5)*this->properties->w, -(this->properties->originY-0.5)*this->properties->h);
  }
  
  if (glfwGetKey(GLFW_KEY_F2)) {
    glColor3f(0.25,0.25,0.25);
    this->DrawBoundingBox();
  }
}

void
Entity::DrawBoundingBox() {
  glPushMatrix();
  glTranslatef(aabb.center.x, aabb.center.y, aabb.center.z);
  glScalef(aabb.extents.x, aabb.extents.y, aabb.extents.z);
  glDisable(GL_CULL_FACE);
    
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE);
  glDepthMask(GL_FALSE);
    
  glBindTexture(GL_TEXTURE_2D, 0);
  drawUnitCube();
  glDepthMask(GL_TRUE);
  glDisable(GL_BLEND);
  glEnable(GL_CULL_FACE);
  glPopMatrix();
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
    this->world->SetCell(this->lastCell->GetPosition(), Cell(this->properties->cellLeave));
  }
  
  // drop inventory
  for (auto item : this->inventory) {
    if (item) {
      std::shared_ptr<Entity> entity(new ItemEntity(item));
      entity->SetPosition(this->aabb.center);
      this->world->AddEntity(entity);
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

