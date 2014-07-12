#ifndef BARFOOS_SPELL_H
#define BARFOOS_SPELL_H

#include "game/gameplay/stats.h"
#include "io/properties.h"

struct Spell : public Properties {

  std::string name = "";

  /** "You cast ..." */
  std::string displayName = "";

  std::vector<Element> incantation;

  std::string onCastAddBuffCaster = "";
  std::string onCastAddBuffAOE    = "";
  std::string onCastAddBuffTarget = "";

  float onCastDamageCaster        = 0.0;
  float onCastDamageAOE           = 0.0;
  float onCastDamageTarget        = 0.0;

  std::string onCastSpawnEntity   = "";

  float castAOERange              = 0.0;
  float castRange                 = 10.0;

  Element element                 = Element::Physical;

  float learnChance               = 0.1;
  bool targetSelf                 = false;
  float castInterval              = 0.0;
  float maxDuration               = 0.0;

  std::string castSound           = "";

  virtual void ParseProperty(const std::string &name) override;

  bool Cast(RunningState &state, Mob &user) const;
};

void LoadSpells();
const Spell &getSpell(const std::vector<Element> &incantation);
const Spell &getSpell(const std::string &name);

#endif
