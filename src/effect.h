#ifndef BARFOOS_EFFECT_H
#define BARFOOS_EFFECT_H

#include "stats.h"
#include "properties.h"
#include "icolor.h"

enum class Beatitude : int8_t;

struct EffectProperties : public Properties {

  std::string name = "";

  /** Display message "You feel ..." when effect is added as buff. */
  std::string feeling = "";

  /** When attached to an identified item, append this to the item's name*/
  std::string displayName = "";

  std::vector<std::string> groups;

  /** Item range factor. */
  float range = 1.0;

  /** Item/mob damage multiplier. */
  float damage = 1.0;

  /** Mob walk speed modifier. */
  float walkSpeed = 1.0;

  /** Item knockback multiplier. */
  float knockback = 1.0;

  int eqAddStr = 0; /**< Add this amount of strength when item equipped. */
  int eqAddDex = 0; /**< Add this amount of dexterity when item equipped. */
  int eqAddAgi = 0; /**< Add this amount of agility when item equipped. */
  int eqAddDef = 0; /**< Add this amount of defense when item equipped. */
  int eqAddHP  = 0; /**< Add this amount of max hp when item equipped. */
  int eqAddMDef = 0;
  int eqAddMAtk = 0;

  int uneqAddStr = 0; /**< Add this amount of strength when item unequipped. */
  int uneqAddDex = 0; /**< Add this amount of dexterity when item unequipped. */
  int uneqAddAgi = 0; /**< Add this amount of agility when item unequipped. */
  int uneqAddDef = 0; /**< Add this amount of defense when item unequipped. */
  int uneqAddHP  = 0; /**< Add this amount of max hp when item unequipped. */
  int uneqAddMDef = 0;
  int uneqAddMAtk = 0;

  /** Item cooldown modifier. */
  float cooldown = 1.0;

  /** Item durability modifier. */
  float durability = 1.0;

  /** Item use damage modifier. */
  float useDurability = 1.0;

  /** Item equip damage modifier. */
  float equipDurability = 1.0;

  /** Block break strength modifier. */
  float breakBlockStrength = 1.0;

  /** Item can remove curses. */
  bool onCombineRemoveCurse = false;

  /** Item can identify other items. */
  bool onCombineIdentify = false;

  /** Destroy item when combined. */
  bool onCombineBreak = false;

  /** Upgrade/downgrade items. */
  int onCombineAddModifier = 0;

  /** Add this amount of health per second to the user. */
  float addHealth = 0;

  /** Effect duration as a buff. */
  float duration = 0.0;

  /** If true when added again the duration of the buff is extended instead
   * of restarted. */
  bool extend = false;

  /** Change item element. */
  Element element = Element::Physical;

  IColor light;

  std::string addSound = "";
  std::string removeSound = "";

  virtual void ParseProperty(const std::string &name) override;

  void ModifyStats(Stats &stats, bool equipped, int modifier, Beatitude beatitude) const;
  void Consume(RunningState &state, Entity &user) const;
  void Update(RunningState &state, Entity &user) const;
};

void LoadEffects();
const EffectProperties &getEffect(const std::string &name);
const std::vector<std::string> &getEffectsInGroup(const std::string &group);

#endif
