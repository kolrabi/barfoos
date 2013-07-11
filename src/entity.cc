#include "entity.h"
#include "world.h"
#include "cell.h"
#include "util.h"
#include "item.h"
#include "itementity.h"
#include "random.h"
#include "game.h"
#include "gfx.h"
#include "input.h"
#include "texture.h"

float Entity::ThinkInterval = 0.2f;

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
    std::vector<std::string> tokens = Tokenize(line);
    if (tokens.size() == 0) continue;
    
    for (auto &c:tokens[0]) c = ::tolower(c);
    
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
    } else if (tokens[0] == "nohit") {
      this->nohit = true;
    } else if (tokens[0] == "nocollideentity") {
      this->nocollideEntity = true;
    } else if (tokens[0] == "nocollidecell") {
      this->nocollideCell = true;
    } else if (tokens[0] == "nocollideowner") {
      this->nocollideOwner = true;
    } else if (tokens[0] == "gravity") {
      this->gravity = std::atof(tokens[1].c_str());
    } else if (tokens[0] == "eyeOffset") {
      this->eyeOffset = std::atof(tokens[1].c_str());
    } else if (tokens[0] == "solid") {
      this->isSolid = true;
    } else if (tokens[0] == "box") {
      this->isBox = true;
    } else if (tokens[0] == "inventory") {
      this->items.push_back({tokens[2], std::atof(tokens[1].c_str())});
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
  this->id = ~0UL;
  this->ownerId = ~0UL;
  this->drawAABB = false;
}

Entity::~Entity() {
}

void
Entity::Start(Game &game, size_t id) {
  World &world = game.GetWorld();
  
  this->id = id;
  
  // fill inventory with random crap
  for (auto item : this->properties->items) {
    if (game.GetRandom().Chance(item.second)) {
      this->AddToInventory(std::shared_ptr<Item>(new Item(item.first)));
      std::cerr << item.first << " added" << std::endl;
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
  aabb.center = aabb.center + offset;
}

void 
Entity::Update(Game &game) {
  float deltaT = game.GetDeltaT();
  
  this->sprite.Update(deltaT);
  
  if (deltaT > 1.0/30.0) {
    this->smoothPosition = aabb.center;
  } else {
    this->smoothPosition = this->smoothPosition + (aabb.center - this->smoothPosition) * game.GetDeltaT() * 30.0f;
  }
  
  for (size_t i=0; i<this->inventory.size(); i++) {
    if (this->inventory[i]) {
      this->inventory[i]->Update(game);
      if (this->inventory[i]->IsRemovable()) {
        this->inventory[i] = nullptr;
      }
    }
  }
  
  this->lastPos = aabb.center;
  this->cellPos = IVector3(aabb.center.x, aabb.center.y, aabb.center.z);
  
  World &world = game.GetWorld();
  Cell *cell = &world.GetCell(cellPos);
  if (cell != this->lastCell) {
    if (this->lastCell && this->properties->cellLeave != "") {
      world.SetCell(this->lastCell->GetPosition(), Cell(this->properties->cellLeave));
    }
    if (this->properties->cellEnter != "") {
      world.SetCell(cellPos, Cell(this->properties->cellEnter));
    }
    
    this->lastCell = cell;
  }
  
  if (cell) {
    this->cellLight = world.GetLight(cell->GetPosition());
  }
  
  this->drawAABB = game.GetInput()->IsKeyActive(InputKey::DebugEntityAABB);
}
  
void 
Entity::Think(Game &game) {
  (void)game;
}

void
Entity::Draw(Gfx &gfx) const {
  gfx.SetColor(this->cellLight, 1.0);
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
  //glBlendFunc(GL_ONE, GL_ONE);
  
  //glDepthMask(GL_FALSE);
    
  gfx.SetTextureFrame(gfx.GetNoiseTexture());
  gfx.SetColor(IColor(64,64,64),0);
  gfx.DrawAABB(this->aabb);
  
  //glDepthMask(GL_TRUE);
  //glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
}

void
Entity::AddHealth(Game &game, const HealthInfo &info) {
  // don't change health of dead or immortal entities
  if (this->health == 0 || this->properties->maxHealth == 0) return;
  
  this->health += info.amount;
  if (this->health <= 0) {
    this->health = 0;
    this->Die(game, info);
  }
}

void
Entity::Die(Game &game, const HealthInfo &info) {
  // TODO: use info (like, displaying a message)
  (void)info;
  
  // TODO: play death animation (if any) and set this->removable afte it finished
  this->removable = true;
  
  if (this->lastCell && this->properties->cellLeave != "") {
    game.GetWorld().SetCell(this->lastCell->GetPosition(), Cell(this->properties->cellLeave));
  }
  
  // drop inventory
  for (auto item : this->inventory) {
    if (item) {
      ItemEntity *entity = new ItemEntity(item);
      // TODO: AABB::PutInside(aabb);
      entity->SetPosition(this->aabb.center);
      
      uint8_t axis;
      entity->aabb.center = game.GetWorld().MoveAABB(entity->aabb, entity->aabb.center + Vector3(game.GetRandom().Float(), game.GetRandom().Float(), game.GetRandom().Float())*aabb.extents, axis);
      entity->AddVelocity(Vector3(game.GetRandom().Float()*0.1, 1, game.GetRandom().Float()*0.1)*10);
      game.AddEntity(entity);
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
    std::cerr << (1<<i) << " " << item->GetProperties()->equippable << std::endl;
    if (i>=(size_t)InventorySlot::Backpack || (item->GetProperties()->equippable & 1<<i)) {
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

