#include "item.h"
#include "entity.h"
#include "world.h"
#include "mob.h"
#include "game.h"
#include "gfx.h"
#include "projectile.h"
#include "texture.h"

static std::map<std::string, ItemProperties> allItems;
ItemProperties defaultItem;

const ItemProperties &getItem(const std::string &name) {
  if (allItems.find(name) == allItems.end()) {
    std::cerr << "entity " << name << " not found" << std::endl;
    return defaultItem;
  }
  return allItems[name];
}

void
ItemProperties::ParseProperty(const std::string &cmd) {
  if (cmd == "tex") {
    Parse("items/texture/", this->sprite.texture);
  } else if (cmd == "frames") {
    Parse(this->sprite.totalFrames);
  } else if (cmd == "anim") {
    size_t firstFrame = 0;
    size_t frameCount = 0;
    float  fps = 0;
    
    Parse(firstFrame);
    Parse(frameCount);
    Parse(fps);
    
    this->sprite.animations.push_back(Animation(firstFrame, frameCount, fps));
    
  } else if (cmd == "size") {
    Parse(this->sprite.width);
    Parse(this->sprite.height);
  } else if (cmd == "damage") {
    Parse(this->damage);
  } else if (cmd == "armorpoints") {
    Parse(this->armor);
  } else if (cmd == "cooldown") {
    Parse(this->cooldown);
  } else if (cmd == "range") {
    Parse(this->range);
  } else if (cmd == "light") {
    Parse(this->light);
    
  } else if (cmd == "flicker") {
    this->flicker = true;
  } else if (cmd == "canusecell") {
    this->canUseCell = true;
  } else if (cmd == "canuseentity") {
    this->canUseEntity = true;
  } else if (cmd == "canusenothing") {
    this->canUseNothing = true;
    
  } else if (cmd == "hand") {
    this->equippable |= (1<<(size_t)InventorySlot::LeftHand) | (1<<(size_t)InventorySlot::RightHand);
  } else if (cmd == "helmet") {
    this->equippable |= (1<<(size_t)InventorySlot::Helmet);
  } else if (cmd == "armor") {
    this->equippable |= (1<<(size_t)InventorySlot::Armor);
  } else if (cmd == "leftring") {
    this->equippable |= (1<<(size_t)InventorySlot::LeftRing);
  } else if (cmd == "rightring") {
    this->equippable |= (1<<(size_t)InventorySlot::RightRing);
  } else if (cmd == "lefthand") {
    this->equippable |= (1<<(size_t)InventorySlot::LeftHand);
  } else if (cmd == "righthand") {
    this->equippable |= (1<<(size_t)InventorySlot::RightHand);
  } else if (cmd == "greaves") {
    this->equippable |= (1<<(size_t)InventorySlot::Greaves);
  } else if (cmd == "boots") {
    this->equippable |= (1<<(size_t)InventorySlot::Boots);
    
  } else if (cmd == "durability") {
    Parse(this->durability);
  } else if (cmd == "usedurability") {
    Parse(this->useDurability);
  } else if (cmd == "equipdurability") {
    Parse(this->equipDurability);
  } else if (cmd == "equipanim") {
    Parse(this->equipAnim);
    
  } else if (cmd == "replacement") {
    Parse(this->replacement);
  } else if (cmd == "onusespawnprojectile") {
  
    Parse(this->spawnProjectile);
  } else if (cmd == "breakblockstrength") {
    Parse(this->breakBlockStrength);
    
  } else if (cmd != "") {
    this->SetError("ignoring '" + cmd + "'");
  }
}

void
LoadItems() {
  std::vector<std::string> assets = findAssets("items");
  for (const std::string &name : assets) {
    FILE *f = openAsset("items/"+name);
    if (f) {
      std::cerr << "loading item " << name << std::endl;
      allItems[name].ParseFile(f);
      fclose(f);
    }
  }
}

Item::Item(const std::string &type) : 
  properties(&getItem(type)),
  durabilityTex(loadTexture("gui/durability")),
  isRemovable(false),
  sprite(this->properties->sprite),
  cooldownFrac(0),
  durability(this->properties->durability),
  isEquipped(false),
  nextUseT(0.0)
{}

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

  if (this->properties->canUseEntity) {
    Entity *entity = game.GetEntity(id);
    if (!entity || entity->GetProperties()->nohit) {
      this->UseOnNothing(game, user);
      return;
    }
    
    entity->AddHealth(game, HealthInfo(-this->properties->damage, HealthType::Melee, Element::Physical, user.GetId()));
    
    this->StartCooldown(game);
  } else {
    this->UseOnNothing(game, user);
  }
}

void Item::UseOnCell(Game &game, Mob &user, Cell *cell, Side side) {
  (void)side;
  
  if (!this->CanUse(game)) return;
  
  if (this->properties->canUseCell) {
    if (game.GetRandom().Chance(this->properties->breakBlockStrength / cell->GetInfo().breakStrength)) {
      cell->GetWorld()->BreakBlock(game, cell->GetPosition());
    }
    this->StartCooldown(game);
  } else {
    this->UseOnNothing(game, user);
  }
}

void Item::UseOnNothing(Game &game, Mob &user) {
  if (!this->CanUse(game)) return;
  this->StartCooldown(game);
  
  if (this->properties->spawnProjectile != "") {
    Projectile *proj = new Projectile(this->properties->spawnProjectile);
    proj->SetOwner(user);
    proj->SetAngles(user.GetAngles());
    proj->SetPosition(user.GetPosition() + Vector3(0,user.GetProperties()->eyeOffset,0));
    proj->AddVelocity(user.GetVelocity());
    game.AddEntity(proj);
  }
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
  
  if (this->properties->durability != this->durability) {
    float dur = this->durability / this->properties->durability;
    int frame = 8 * dur;
    gfx.SetTextureFrame(this->durabilityTex, 0, frame, 8);
    gfx.DrawIconQuad(p);
  }
}

void Item::DrawSprite(Gfx &gfx, const Vector3 &pos) const {
  gfx.DrawSprite(this->sprite, pos);
}
