#include "item.h"
#include "entity.h"
#include "world.h"
#include "mob.h"

#include <GL/glfw.h>

#include <cstring>

std::vector<std::string> Tokenize(const char *l) {
  std::vector<std::string> tokens;
  
  std::string line = l;
  char *p = &line[0];
  char *q;
  
  // skip whitespace
  while(*p && strchr(" \r\n\t", *p)) p++;
  
  // end of string or first character is #?
  if (*p == 0 || *p == '#') return tokens;
  
  do {
    // find end of token
    q = p;
    while(*q && !strchr(" \r\n\t", *q)) q++;
    
    // terminate token
    if (q) *q = 0;
    tokens.push_back(p);
    
    // not end of line? then skip whitespace to next token
    if (q) { 
      p = q+1; 
      while(*p && strchr(" \r\n\t", *p)) p++;
    }
  } while(q && *p);
  return tokens;
}

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
    std::vector<std::string> tokens = Tokenize(line);
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
      this->equippable |= (1<<(size_t)InventorySlot::LeftHand) | (1<<(size_t)InventorySlot::RightHand);
    } else if (tokens[0] == "helmet") {
      this->equippable |= (1<<(size_t)InventorySlot::Helmet);
    } else if (tokens[0] == "armor") {
      this->equippable |= (1<<(size_t)InventorySlot::Armor);
    } else if (tokens[0] == "leftring") {
      this->equippable |= (1<<(size_t)InventorySlot::LeftRing);
    } else if (tokens[0] == "rightring") {
      this->equippable |= (1<<(size_t)InventorySlot::RightRing);
    } else if (tokens[0] == "lefthand") {
      this->equippable |= (1<<(size_t)InventorySlot::LeftHand);
    } else if (tokens[0] == "righthand") {
      this->equippable |= (1<<(size_t)InventorySlot::RightHand);
    } else if (tokens[0] == "greaves") {
      this->equippable |= (1<<(size_t)InventorySlot::Greaves);
    } else if (tokens[0] == "boots") {
      this->equippable |= (1<<(size_t)InventorySlot::Boots);
    } else if (tokens[0] == "durability") {
      this->durability = std::atof(tokens[1].c_str());
    } else if (tokens[0] == "usedurability") {
      this->useDurability = std::atof(tokens[1].c_str());
    } else if (tokens[0] == "equipdurability") {
      this->equipDurability = std::atof(tokens[1].c_str());
    } else if (tokens[0] == "equipanim") {
      this->equipAnim = std::atoi(tokens[1].c_str());
    } else if (tokens[0] == "replacement") {
      this->replacement = tokens[1];
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
      animation = this->IsEquipped()?this->properties->equipAnim:0;
      frame = frame - (int)frame + this->properties->anims[0].firstFrame;
    }
  }
  
  if (this->isEquipped) { 
    this->durability -= this->properties->equipDurability * deltaT;
  }
  
  if (this->durability <= 0 && this->properties->durability != 0.0) {
    std::string replacement = this->properties->replacement;
    if (replacement != "") {
      bool isEquipped = this->isEquipped;
      *this = Item(replacement);
      this->isEquipped = isEquipped;
      return;
    } else {
      this->isRemovable = true;
    }
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
  
  cell->GetWorld()->BreakBlock(cell->GetPosition());
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

void Item::DrawSprite(const Vector3 &p, float w, float h) const {
  float u = 0.0;
  float uw = 1.0;
  if (this->properties->frames) {
    int f = ((int)this->frame) % this->properties->frames;
    uw = 1.0/this->properties->frames;
    u = f*uw;
  }

  drawBillboard(p, w, h, this->properties->texture, u, uw);
}
