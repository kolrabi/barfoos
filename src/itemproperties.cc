#include "item.h"
#include "entity.h"
#include "world.h"
#include "gfx.h"
#include "texture.h"
#include "text.h"
#include "game.h"

#include <unordered_map>
#include <cstdio>

static std::unordered_map<std::string, ItemProperties> allItems;
static std::unordered_map<std::string, std::vector<std::string>> allItemGroups;
ItemProperties defaultItem;

const ItemProperties &getItem(const std::string &name) {
  if (name == "default") return defaultItem;
  if (allItems.find(name) == allItems.end()) {
    Log("Properties for entity of type '%s' not found\n", name.c_str());
    return defaultItem;
  }
  return allItems[name];
}

std::string getRandomItem(const std::string &group, Random &random) {
  const std::vector<std::string> &groupItems = allItemGroups[group];

  if (groupItems.empty()) {
    Log("No entity properties in group '%s' found\n", group.c_str());
    return "default";
  }

  return groupItems[random.Integer(groupItems.size())];
}

static void shuffleItems(Game &game, std::function<bool(ItemProperties&)> doShuffleFn) {
  std::vector<std::string> items;
  for (auto &i : allItems) {
    if (doShuffleFn(i.second)) {
      items.push_back(i.first);
    }
  }

  for (size_t i = 0; i < items.size(); i++) {
    // swap appearances and descriptions
    size_t j = game.GetRandom().Integer(items.size());
    std::swap(allItems[items[i]].sprite,           allItems[items[j]].sprite);
    std::swap(allItems[items[i]].unidentifiedName, allItems[items[j]].unidentifiedName);
  }
}

void
ItemProperties::ParseProperty(const std::string &cmd) {
  if (cmd == "tex") {
    Parse("items/texture/", this->sprite.texture);
  } else if (cmd == "frames") {
    Parse(this->sprite.totalFrames);
  } else if (cmd == "group") {
    Parse(this->groups);
  } else if (cmd == "anim") {
    uint32_t firstFrame = 0;
    uint32_t frameCount = 0;
    float  fps = 0;

    Parse(firstFrame);
    Parse(frameCount);
    Parse(fps);

    this->sprite.animations.push_back(Animation(firstFrame, frameCount, fps));
    if (this->sprite.animations.size() == 1) this->sprite.StartAnim(0);

  } else if (cmd == "size") {
    Parse(this->sprite.width);
    Parse(this->sprite.height);
  }

  else if (cmd == "stab")       this->useMovement = UseMovement::StabMovement;
  else if (cmd == "recoil")     this->useMovement = UseMovement::RecoilMovement;
  else if (cmd == "damage")     Parse(this->damage);
  else if (cmd == "knockback")  Parse(this->knockback);

  else if (cmd == "nomodifier")     this->noModifier = true;
  else if (cmd == "nobeatitude")    this->noBeatitude = true;

  else if (cmd == "eqstr")      Parse(this->eqAddStr);
  else if (cmd == "eqdex")      Parse(this->eqAddDex);
  else if (cmd == "eqagi")      Parse(this->eqAddAgi);
  else if (cmd == "eqdef")      Parse(this->eqAddDef);
  else if (cmd == "eqhp")       Parse(this->eqAddHP);

  else if (cmd == "uneqstr")    Parse(this->uneqAddStr);
  else if (cmd == "uneqdex")    Parse(this->uneqAddDex);
  else if (cmd == "uneqagi")    Parse(this->uneqAddAgi);
  else if (cmd == "uneqdef")    Parse(this->uneqAddDef);
  else if (cmd == "uneqhp")     Parse(this->uneqAddHP);

  else if (cmd == "light")      Parse(this->light);
  else if (cmd == "flicker")    this->flicker = true;

  else if (cmd == "cooldown")       Parse(this->cooldown);
  else if (cmd == "range")          Parse(this->range);
  else if (cmd == "canusecell")     this->canUseCell = true;
  else if (cmd == "canuseentity")   this->canUseEntity = true;
  else if (cmd == "canusenothing")  this->canUseNothing = true;
  else if (cmd == "onusespawnprojectile") Parse(this->spawnProjectile);
  else if (cmd == "breakblockstrength") Parse(this->breakBlockStrength);

  else if (cmd == "hand")           this->equippable |= (1<<(size_t)InventorySlot::LeftHand) | (1<<(size_t)InventorySlot::RightHand);
  else if (cmd == "lefthand")       this->equippable |= (1<<(size_t)InventorySlot::LeftHand);
  else if (cmd == "righthand")      this->equippable |= (1<<(size_t)InventorySlot::RightHand);
  else if (cmd == "helmet")         this->equippable |= (1<<(size_t)InventorySlot::Helmet);
  else if (cmd == "armor")          this->equippable |= (1<<(size_t)InventorySlot::Armor);
  else if (cmd == "ring")           { this->equippable |= (1<<(size_t)InventorySlot::LeftRing) | (1<<(size_t)InventorySlot::RightRing); this->isRing = true; }
  else if (cmd == "amulet")         { this->equippable |= (1<<(size_t)InventorySlot::Amulet); this->isAmulet = true; }
  else if (cmd == "leftring")       { this->equippable |= (1<<(size_t)InventorySlot::LeftRing); this->isRing = true; }
  else if (cmd == "rightring")      { this->equippable |= (1<<(size_t)InventorySlot::RightRing); this->isRing = true; }
  else if (cmd == "greaves")        this->equippable |= (1<<(size_t)InventorySlot::Greaves);
  else if (cmd == "boots")          this->equippable |= (1<<(size_t)InventorySlot::Boots);

  else if (cmd == "stack")          this->stackable = true;

  else if (cmd == "durability")       Parse(this->durability);
  else if (cmd == "usedurability")    Parse(this->useDurability);
  else if (cmd == "equipdurability")  Parse(this->equipDurability);
  else if (cmd == "combinedurability")  Parse(this->combineDurability);
  else if (cmd == "equipanim")        Parse(this->equipAnim);

  else if (cmd == "replacement")      Parse(this->replacement);

  else if (cmd == "identifiedname")   Parse(this->identifiedName);
  else if (cmd == "unidentifiedname") Parse(this->unidentifiedName);
  else if (cmd == "name") {
    Parse(this->unidentifiedName);
    this->identifiedName = this->unidentifiedName;
  }
  else if (cmd == "scroll")         this->unidentifiedName = this->game->GetScrollName();
  else if (cmd == "potion")         this->isPotion = true;
  else if (cmd == "wand")           this->isWand = true;

  else if (cmd == "onuseidentify")  this->onUseIdentify = true;
  else if (cmd == "oncombineeffect")  Parse(this->onCombineEffect);
  else if (cmd == "onconsumeeffect")  Parse(this->onConsumeEffect);
  else if (cmd == "onconsumeresult")  Parse(this->onConsumeResult);
  else if (cmd == "onconsumeaddbuff") Parse(this->onConsumeAddBuff);
  else if (cmd == "onconsumeteleport") this->onConsumeTeleport = true;
  else if (cmd == "onhitaddbuff") {
    std::string effect;
    Parse(effect);
    float prob;
    Parse(prob);
    this->onHitAddBuff[effect] = prob;

  } else if (cmd == "effect") {
    float w;
    Parse(w);

    std::string name;
    Parse(name);

    this->effects[name] = w;

  } else if (cmd == "unlockchance") {
    Parse(this->unlockChance);
  } else if (cmd == "onunlockbreak") {
    this->onUnlockBreak = true;
  } else if (cmd != "") {
    this->SetError("ignoring '" + cmd + "'");
  }
}

void
LoadItems(Game &game) {
  allItems.clear();

  std::vector<std::string> assets = findAssets("items");
  for (const std::string &name : assets) {
    FILE *f = openAsset("items/"+name);
    if (f) {
      allItems[name].game = &game;
      allItems[name].name = name;
      allItems[name].identifiedName = name;
      allItems[name].unidentifiedName = name;
      allItems[name].groups.push_back(name);
      allItems[name].ParseFile(f);
      for (auto &g : allItems[name].groups) {
        allItemGroups[g].push_back(name);
      }
      fclose(f);
    }
  }

  shuffleItems(game, [](ItemProperties &p){return p.isPotion;});
  shuffleItems(game, [](ItemProperties &p){return p.isWand;});
  shuffleItems(game, [](ItemProperties &p){return p.isRing;});
  shuffleItems(game, [](ItemProperties &p){return p.isAmulet;});
}
