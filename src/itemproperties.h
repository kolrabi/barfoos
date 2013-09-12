#ifndef BARFOOS_ITEMPROPERTIES_H
#define BARFOOS_ITEMPROPERTIES_H

#include "common.h"
#include "icolor.h"
#include "sprite.h"

#include "effect.h"
#include "properties.h"

enum class UseMovement : uint8_t {
  SlashMovement,
  StabMovement,
  RecoilMovement
};

struct ItemProperties : public Properties {
  std::string name = "<item>";
  std::string identifiedName = "<item>";
  std::string unidentifiedName = "<item>";
  std::vector<std::string> groups;

  int     minLevel          = 0;
  int     maxLevel          = -1;
  float   maxProbability    = 1.0;

  bool isPotion = false;
  bool isWand   = false;
  bool isRing   = false;
  bool isAmulet = false;
  bool isScroll = false;

  // rendering
  Sprite sprite = Sprite();
  ID equipAnim = 0;
  ID chargeAnim = InvalidID;
  IColor light {0,0,0};
  bool flicker = false;
  UseMovement useMovement = UseMovement::SlashMovement;

  // gameplay
  float range = 5.0;
  float damage = 1.0;
  float knockback = 0.0;

  int eqAddStr = 0;
  int eqAddDex = 0;
  int eqAddAgi = 0;
  int eqAddVit = 0;
  int eqAddInt = 0;
  int eqAddLuk = 0;

  int eqAddAtk = 0;
  int eqAddDef = 0;
  int eqAddMDef = 0;
  int eqAddMAtk = 0;

  int eqAddHit = 0;
  int eqAddCrit = 0;
  int eqAddFlee = 0;

  int eqAddHP  = 0;

  int uneqAddStr = 0;
  int uneqAddDex = 0;
  int uneqAddAgi = 0;
  int uneqAddVit = 0;
  int uneqAddInt = 0;
  int uneqAddLuk = 0;

  int uneqAddAtk  = 0;
  int uneqAddDef = 0;
  int uneqAddMDef = 0;
  int uneqAddMAtk = 0;

  int uneqAddHit = 0;
  int uneqAddCrit = 0;
  int uneqAddFlee = 0;

  int uneqAddHP  = 0;

  std::string useSkill = "";

  uint32_t equippable = 0;
  bool twoHanded = false;

  float cooldown = 1.0;

  float durability = 10;
  float useDurability = 0.0;
  float equipDurability = 0.0;
  float combineDurability = 0.0;

  bool canUseCell = false;
  bool canUseEntity = false;
  bool canUseNothing = false;
  bool noModifier = false;
  bool noBeatitude = false;
  float chargeTime = 0.0f;

  std::string onCombineEffect = "";
  std::unordered_map<std::string, std::string> combinations;

  std::string onConsumeEffect = "";
  std::string onConsumeVerb = "";
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
  bool onUnlockBreak = false;
  bool pickLiquid = false;
  
  virtual void ParseProperty(const std::string &name) override;
};

void LoadItems(Game &game);
const ItemProperties &getItem(const std::string &name);
std::string getRandomItem(const std::string &group, int level, Random &random);
float GetItemProbability(const std::string &type, int level);

#endif

