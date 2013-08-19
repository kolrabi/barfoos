#ifndef BARFOOS_ITEMPROPERTIES_H
#define BARFOOS_ITEMPROPERTIES_H

#include "common.h"
#include "icolor.h"
#include "sprite.h"

#include "effect.h"
#include "properties.h"

struct ItemProperties : public Properties {
  std::string name = "<item>";
  std::string identifiedName = "<item>";
  std::string unidentifiedName = "<item>";
  std::vector<std::string> groups;
  
  bool isPotion = false;
  bool isWand   = false;
  bool isRing   = false;
  bool isAmulet = false;
  
  // rendering
  Sprite sprite = Sprite();
  uint32_t equipAnim = 0;
  IColor light {0,0,0};
  bool flicker = false;

  // gameplay
  float range = 5.0;
  float damage = 1.0;
  float knockback = 0.0;
  
  int eqAddStr = 0;
  int eqAddDex = 0;
  int eqAddAgi = 0;
  int eqAddDef = 0;
  int eqAddHP  = 0;

  int uneqAddStr = 0;
  int uneqAddDex = 0;
  int uneqAddAgi = 0;
  int uneqAddDef = 0;
  int uneqAddHP  = 0;
  
  std::string weaponClass = "";
  
  uint32_t equippable = 0;
  bool twoHanded = false;
  
  float cooldown = 1.0;
  
  float durability = 10;
  float useDurability = 0.0;
  float equipDurability = 0.0;
  
  bool canUseCell = false;
  bool canUseEntity = false;
  bool canUseNothing = false;
  bool noModifier = false;
  bool noBeatitude = false;
  
  std::string onCombineEffect = "";
  std::string onConsumeEffect = "";
  std::string onConsumeResult = "";
  std::string onConsumeAddBuff = "";
  bool onConsumeTeleport = false;

  bool onUseIdentify = false;

  weighted_map<std::string> onHitAddBuff;
  
  float breakBlockStrength = 0.0;
  
  std::string replacement = "";
  
  // std::string placeEntity = "";
  std::string spawnProjectile = "";
  
  weighted_map<std::string> effects = weighted_map<std::string>();
  bool stackable = false;
 
  float unlockChance = 0.0;
  virtual void ParseProperty(const std::string &name) override;
};

void LoadItems(Game &game);
const ItemProperties &getItem(const std::string &name);
std::string getRandomItem(const std::string &group, Random &random);

#endif

