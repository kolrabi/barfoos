#include "item.h"
#include "entity.h"
#include "world.h"
#include "mob.h"
#include "game.h"
#include "gfx.h"

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

static std::map<std::string, ItemProperties> allItems;
ItemProperties defaultItem;

const ItemProperties *getItem(const std::string &name) {
  if (allItems.find(name) == allItems.end()) {
    std::cerr << "entity " << name << " not found" << std::endl;
    return &defaultItem;
  }
  return &allItems[name];
}

ItemProperties::ItemProperties() {
}

ItemProperties::ItemProperties(FILE *f) {
  char line[256];
  
  while(fgets(line, 256, f) && !feof(f)) {
    std::vector<std::string> tokens = Tokenize(line);
    if (tokens.size() == 0) continue;
    
    for (auto &c:tokens[0]) c = ::tolower(c);
    
    if (tokens[0] == "tex") {
      this->sprite.texture = loadTexture("items/texture/"+tokens[1]);
    } else if (tokens[0] == "frames") {
      this->sprite.totalFrames = std::atoi(tokens[1].c_str());
    } else if (tokens[0] == "anim") {
      this->sprite.animations.push_back(Animation(std::atoi(tokens[1].c_str()), std::atoi(tokens[2].c_str()), std::atof(tokens[3].c_str())));
    } else if (tokens[0] == "size") {
      this->sprite.width = std::atof(tokens[1].c_str());
      this->sprite.height = std::atof(tokens[2].c_str());
    } else if (tokens[0] == "damage") {
      this->damage = std::atof(tokens[1].c_str());
    } else if (tokens[0] == "armorpoints") {
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
      allItems[name] = ItemProperties(f);
      fclose(f);
    }
  }
}

Item::Item(const std::string &type) {
  this->properties = getItem(type);
  this->sprite = this->properties->sprite;
  this->nextUseT = 0;
  this->isEquipped = false;
  this->isRemovable = false;
  this->durability = this->properties->durability;
  this->cooldownFrac = 0;
}

Item::~Item() {
}

bool Item::CanUse(Game &game) const {
  return (this->durability > 0 || this->properties->durability == 0.0) && 
         this->properties->cooldown >= 0.0 && 
         this->nextUseT < game.GetTime();
}
  
void Item::StartCooldown(Game &game) {
  this->durability -= this->properties->useDurability;
  this->nextUseT = game.GetTime() + this->properties->cooldown;
}

void Item::Update(Game &game) {
  // reduce durability while equipped  
  if (this->isEquipped) { 
    this->sprite.currentAnimation = this->properties->equipAnim;
    this->durability -= this->properties->equipDurability * game.GetDeltaT();
  }

  this->sprite.Update(game.GetDeltaT());
  
  // when broken, replace or remove
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
  
  // cooldown fraction: 1.0 full cooldown left, 0.0 cooldown over
  cooldownFrac = (nextUseT - game.GetTime())/this->properties->cooldown;
  if (cooldownFrac < 0) cooldownFrac = 0;
}

void Item::UseOnEntity(Game &game, Mob &user, size_t id) {
  if (!this->CanUse(game)) return;
  
  temp_ptr<Entity> entity(game.GetEntity(id));
  if (!entity) return;
  
  entity->AddHealth(game, HealthInfo(-this->properties->damage, HealthType::Melee, user.GetId()));
  
  this->StartCooldown(game);
}

void Item::UseOnCell(Game &game, Mob &user, Cell *cell, Side side) {
  (void)user;
  (void)side;
  
  if (!this->CanUse(game)) return;
  
  cell->GetWorld()->BreakBlock(game, cell->GetPosition());
  this->StartCooldown(game);
}

void Item::UseOnNothing(Game &game, Mob &user) {
  (void)user;
  if (!this->CanUse(game)) return;
  this->StartCooldown(game);
}

void Item::Draw(Gfx &gfx, bool left) {
  gfx.GetView().Push();
  gfx.SetBackfaceCulling(false);
  gfx.GetView().Scale(Vector3(left ? 1 : -1, 1, 1));
  gfx.GetView().Translate(Vector3(1, -2, 4));
  
  gfx.GetView().Rotate(40, Vector3(0, 1, 0));

  gfx.GetView().Translate(Vector3(1,-1,0));
  gfx.GetView().Rotate(cooldownFrac*60-60, Vector3(0,0,1));
  gfx.GetView().Translate(Vector3(-1,1,0));
  gfx.GetView().Scale(Vector3(2,2,2));
  gfx.DrawSprite(this->sprite, Vector3(0,0,0), false);
  gfx.SetBackfaceCulling(true);
  gfx.GetView().Pop();
}

void Item::DrawIcon(Gfx &gfx, const Point &p) const {
  gfx.DrawIcon(this->sprite, p);
}

void Item::DrawSprite(Gfx &gfx, const Vector3 &pos) const {
  gfx.DrawSprite(this->sprite, pos);
}
