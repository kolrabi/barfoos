#include "item.h"
#include "entity.h"
#include "world.h"
#include "mob.h"

#include <GL/glfw.h>

#include <cstring>

static std::map<std::string, ItemProperties *> allItems;
ItemProperties defaultItem;

const ItemProperties *getItem(const std::string &name) {
  if (allItems.find(name) == allItems.end()) {
    std::cerr << "entity " << name << " not found" << std::endl;
    return &defaultItem;
  }
  return allItems[name];
}

ItemProperties::ItemProperties() {
}

ItemProperties::ItemProperties(FILE *f) {
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
      this->texture = loadTexture("items/texture/"+tokens[1]);
    } else if (tokens[0] == "frames") {
      this->frames = std::atoi(tokens[1].c_str());
    } else if (tokens[0] == "anim") {
      this->anims.push_back(Animation(std::atoi(tokens[1].c_str()), std::atoi(tokens[2].c_str()), std::atof(tokens[3].c_str())));
    } else if (tokens[0] == "damage") {
      this->damage = std::atof(tokens[1].c_str());
    } else if (tokens[0] == "armor") {
      this->armor = std::atof(tokens[1].c_str());
    } else if (tokens[0] == "cooldown") {
      this->cooldown = std::atof(tokens[1].c_str());
    } else if (tokens[0] == "range") {
      this->range = std::atof(tokens[1].c_str());
    } else if (tokens[0] == "light") {
      this->light = IColor(std::atoi(tokens[1].c_str()), std::atoi(tokens[2].c_str()), std::atoi(tokens[3].c_str()));
    } else if (tokens[0] == "flicker") {
      this->flicker = true;
    } else if (tokens[0] == "hand") {
      this->equippable = (1<<(size_t)InventorySlot::LeftHand) || (1<<(size_t)InventorySlot::RightHand);
      // TODO: other slots...
    } else if (tokens[0] == "durability") {
      this->durability = std::atof(tokens[1].c_str());
    } else if (tokens[0] == "usedurability") {
      this->useDurability = std::atof(tokens[1].c_str());
    } else if (tokens[0] == "equipdurability") {
      this->equipDurability = std::atof(tokens[1].c_str());
    } else if (tokens[0] != "") {
      std::cerr << "ignoring '" << tokens[0] << "'" << std::endl;
    }
  }
}

void
LoadItems() {
  std::vector<std::string> assets = findAssets("items");
  for (const std::string &name : assets) {
    FILE *f = openAsset("items/"+name);
    if (f) {
      std::cerr << "loading item " << name << std::endl;
      allItems[name] = new ItemProperties(f);
      fclose(f);
    }
  }
}

Item::Item(const std::string &type) {
  this->properties = getItem(type);
  this->nextUseT = 0;
  this->lastT = 0;
  this->isEquipped = false;
  this->animation = 0;
  this->frame = 0;
  this->isRemovable = false;
  this->durability = this->properties->durability;
}

Item::~Item() {
}

bool Item::CanUse() const {
  return (this->durability > 0 || this->properties->durability == 0.0) && 
         this->properties->cooldown >= 0.0 && 
         this->nextUseT < glfwGetTime();
}
  
void Item::StartCooldown() {
  this->durability -= this->properties->useDurability;
  this->nextUseT = glfwGetTime() + this->properties->cooldown;
}

void Item::Update(float t) {
  if (this->lastT == 0.0) this->lastT = t;
  float deltaT = t - lastT;
  this->lastT = t;

  if (this->properties->anims.size() > 0) {
    const Animation &a = this->properties->anims[animation];
    frame += a.fps * deltaT;
    if (frame >= a.frameCount+a.firstFrame) {
      animation = 0;
      frame = frame - (int)frame + this->properties->anims[0].firstFrame;
    }
  }
  
  if (this->isEquipped) { 
    this->durability -= this->properties->equipDurability * deltaT;
  }
  
  if (this->durability <= 0 && this->properties->durability != 0.0) {
    this->isRemovable = true;
  }
}

void Item::UseOnEntity(const std::shared_ptr<Entity> &entity) {
  if (!this->CanUse()) return;
  
  entity->AddHealth(-this->properties->damage);
  
  this->StartCooldown();
}

void Item::UseOnCell(Cell *cell, Side side) {
  (void)side;
  if (!this->CanUse()) return;
  
  cell->GetWorld()->SetCell(cell->GetPosition(), Cell("air"));
  
  this->StartCooldown();
}

void Item::Draw(bool left) {
  float u = 0.0;
  float uw = 1.0;
  if (this->properties->frames) {
    int f = ((int)this->frame) % this->properties->frames;
    uw = 1.0/this->properties->frames;
    u = f*uw;
  }

  glPushMatrix();
  
  glDisable(GL_CULL_FACE);
  glScalef(left ? -1 : 1, 1, 1);
  glTranslatef(-1, -2, 4);
  glRotatef(-60, 0, 1, 0);

  // cooldown fraction: 1.0 full cooldown left, 0.0 cooldown over
  float f = (nextUseT - lastT)/this->properties->cooldown;
  if (f < 0) f = 0;

  glTranslatef(-1,-1,0);
  glRotatef(60-f*60, 0,0,1);
  glTranslatef(1,1,0);
  
  glBindTexture(GL_TEXTURE_2D, this->properties->texture);
  glBegin(GL_QUADS);
  glTexCoord2f(u+uw,1); glVertex3f(-1, 1,0);
  glTexCoord2f(u,1); glVertex3f( 1, 1,0);
  glTexCoord2f(u,0); glVertex3f( 1,-1,0);
  glTexCoord2f(u+uw,0); glVertex3f(-1,-1,0);
  glEnd();
  
  glPopMatrix();
}

void Item::DrawIcon(const Point &p) const {
  float u = 0.0;
  float uw = 1.0;
  if (this->properties->frames) {
    int f = ((int)this->frame) % this->properties->frames;
    uw = 1.0/this->properties->frames;
    u = f*uw;
  }

  drawIcon(p, Point(32,32), this->properties->texture, u, uw);
}
