#include "common.h"

#include "audio/audio.h"
#include "game/entities/player.h"
#include "game/gameplay/spell.h"
#include "game/gamestates/running/runningstate.h"
#include "game/items/item.h"
#include "game/items/itementity.h"
#include "game/world/cells/cell.h"
#include "game/world/world.h"
#include "gfx/gfx.h"
#include "gfx/gfxview.h"
#include "gfx/shader.h"
#include "gfx/text.h"
#include "gfx/texture.h"
#include "gfx/vertex.h"
#include "io/input.h"

Player::Player() :
  Mob("player"),

  // rendering
  crosshairTex      (Texture::Get("gui/crosshair")),
  slotTex           (Texture::Get("gui/slot")),

  // gameplay
  itemActiveLeft    (false),
  itemActiveRight   (false),
  lastItemActiveLeft (false),
  lastItemActiveRight(false),

  // display
  messages          (),

  fps               (0.0),

  bigMessage        (new RenderString("", "big")),
  bigMessageT       (0.0),

  mapZoom           (-32.0f),

  leftHand          (new Item("barehand.player")),
  rightHand         (new Item("barehand.player")),

  blink(false)
{
  this->proto.set_spawn_class(uint32_t(SpawnClass::PlayerClass));
  this->proto.mutable_player();

  // TEST:
  this->inventory.Equip(std::make_shared<Item>(Item("sword")), InventorySlot::RightHand);
  this->inventory.Equip(std::make_shared<Item>(Item("torch")), InventorySlot::LeftHand);
  this->inventory.AddToBackpack(std::make_shared<Item>(Item("torch")));

  this->baseStats.UpgradeSkill("magic", 10);

  //this->LearnSpell("spell.test");

  gemSprites[Element::Physical] = Sprite("items/texture/gem.empty");
  gemSprites[Element::Fire]     = Sprite("items/texture/gem.fire");
  gemSprites[Element::Earth]    = Sprite("items/texture/gem.earth");
  gemSprites[Element::Wind]      = Sprite("items/texture/gem.wind");
  gemSprites[Element::Water]    = Sprite("items/texture/gem.water");
  gemSprites[Element::Life]     = Sprite("items/texture/gem.life");
}

Player::Player(const Entity_Proto &proto) :
  Mob(proto),

  // rendering
  crosshairTex      (Texture::Get("gui/crosshair")),
  slotTex           (Texture::Get("gui/slot")),

  // gameplay
  itemActiveLeft    (false),
  itemActiveRight   (false),
  lastItemActiveLeft (false),
  lastItemActiveRight(false),

  // display
  messages          (),

  fps               (0.0),

  bigMessage        (new RenderString("", "big")),
  bigMessageT       (0.0),

  mapZoom           (-32.0f),

  leftHand          (new Item("barehand.player")),
  rightHand         (new Item("barehand.player")),

  blink(false)
{
  for (auto &m:this->proto.player().messages()) {
    Message *msg = new Message(m.text(), m.font());
    msg->messageTime = m.time();
    this->messages.push_back(msg);
  }

  for (auto &e:this->proto.player().elements()) {
    this->elements.push_back(Element(e));
  }
}

Player::~Player() {
  delete this->bigMessage;
}

void
Player::View(Gfx &gfx) const {
  Vector3 fwd   = this->GetForward();
  Vector3 right = this->GetRight();
  Vector3 bob   = Vector3(0,1,0) * std::abs(std::sin(this->GetBobPhase() * Const::pi * 2)*0.05) * this->GetBobAmplitude() +
                  right          *          std::cos(this->GetBobPhase() * Const::pi * 2)*0.05  * this->GetBobAmplitude();

  Vector3 pos   = this->smoothPosition + Vector3(0,this->properties->eyeOffset,0) + bob;

  gfx.GetView().Look(pos, fwd);
}

void
Player::MapView(Gfx &gfx) const {
  gfx.GetView().Look(Vector3(0,0,-1), Vector3(0,0,1), -1, Vector3(0,1,0));
}

void
Player::Start(RunningState &state, uint32_t id) {
  Mob::Start(state, id);
  this->SetPain(0.0f);
}

void
Player::Update(RunningState &state) {
  Mob::Update(state);

  Game &game = state.GetGame();
  float deltaT = game.GetDeltaT();

  this->fps = game.GetFPS();

  ID entityID;
  Side cellSide;
  this->GetSelection(state, 5.0, nullptr, cellSide, entityID);
  this->lookAtEntity = state.GetEntity(entityID);

  this->blink = std::fmod(game.GetTime()*2, 1.0) > 0.3;
  this->SetHPFlashTime(std::max(0.0f, this->GetHPFlashTime() - game.GetDeltaT()));

  this->SetPain(std::max(0.0f, this->GetPain() - game.GetDeltaT() * 0.1f));

  if (!this->IsInLiquid() && move.GetMag() > 1.5) {
    this->SetBobAmplitude(this->GetBobAmplitude() + deltaT * 4);
    if (this->GetBobAmplitude() > 1.0) this->SetBobAmplitude(1.0f);
  } else {
    this->SetBobAmplitude(this->GetBobAmplitude() - deltaT*4);
    if (this->GetBobAmplitude() <= 0.0) {
      this->SetBobAmplitude(0.0f);
      this->SetBobPhase(1.0f);
    }
  }

  float lastPhase = this->GetBobPhase();
  if (this->GetBobAmplitude() > 0.0f) this->SetBobPhase(this->GetBobPhase() + deltaT);
  if (this->GetBobPhase() > 1.0f) {
    this->SetBobPhase(this->GetBobPhase() - 1.0f);

    // TODO: get step sound from ground cell
    if (this->groundCell) {
      float pitch = 1.0 + state.GetRandom().Float()*0.05;
      std::string name = "step_a";
      switch(state.GetRandom().Integer(4)) {
        case 0: name = "step_a"; break;
        case 1: name = "step_b"; break;
        case 2: name = "step_c"; break;
        case 3: name = "step_d"; break;
      }
      state.GetGame().GetAudio().PlaySound(name, this->GetSmoothPosition() - (this->GetRight()*0.2), pitch);
    }
  } else if (this->GetBobPhase() >= 0.5 && lastPhase < 0.5) {
    // TODO: get step sound from ground cell
    if (this->groundCell) {
      float pitch = 1.0 + state.GetRandom().Float()*0.05;
      std::string name = "step_a";
      switch(state.GetRandom().Integer(4)) {
        case 0: name = "step_a"; break;
        case 1: name = "step_b"; break;
        case 2: name = "step_c"; break;
        case 3: name = "step_d"; break;
      }
      state.GetGame().GetAudio().PlaySound(name, this->GetSmoothPosition() + (this->GetRight()*0.2), pitch);
    }
  }

  this->UpdateInput(state);

  float   speed = this->properties->maxSpeed * this->GetMoveModifier();
  if (move.GetMag() > speed) move = move.Normalize() * speed;

  this->rightHand->Update(state);
  this->leftHand->Update(state);

  if (itemActiveLeft) {
    std::shared_ptr<Item> useItem = this->inventory[InventorySlot::RightHand];
    if (!useItem) useItem = this->rightHand;

    if (useItem->NeedsChargeUp()) {
      useItem->SetCharging(true);
    } else {
      std::string skill = useItem->GetProperties().useSkill;
      bool result = this->UseItem(state, useItem);
      if (result && skill != "") {
        if (this->baseStats.UpgradeSkill(skill)) {
          // TODO: skill level up message
        }
      }
    }
  } else {
    std::shared_ptr<Item> useItem = this->inventory[InventorySlot::RightHand];
    if (!useItem) useItem = this->rightHand;

    if (useItem->NeedsChargeUp() && lastItemActiveLeft) {
      std::string skill = useItem->GetProperties().useSkill;
      bool result = this->UseItem(state, useItem);
      if (result && skill != "") {
        if (this->baseStats.UpgradeSkill(skill)) {
          // TODO: skill level up message
        }
      }
    }
  }
  lastItemActiveLeft = itemActiveLeft;

  if (itemActiveRight) {
    std::shared_ptr<Item> useItem = this->inventory[InventorySlot::LeftHand];
    if (!useItem) useItem = this->leftHand;
    if (useItem->NeedsChargeUp()) {
      useItem->SetCharging(true);
    } else {
      std::string skill = useItem->GetProperties().useSkill;
      bool result = this->UseItem(state, useItem);
      if (result && skill != "") {
        if (this->baseStats.UpgradeSkill(skill)) {
          // TODO: skill level up message
        }
      }
    }
  } else {
    std::shared_ptr<Item> useItem = this->inventory[InventorySlot::LeftHand];
    if (!useItem) useItem = this->leftHand;

    if (useItem->NeedsChargeUp() && lastItemActiveRight) {
      std::string skill = useItem->GetProperties().useSkill;
      bool result = this->UseItem(state, useItem);
      if (result && skill != "") {
        if (this->baseStats.UpgradeSkill(skill)) {
          // TODO: skill level up message
        }
      }
    }
  }
  lastItemActiveRight = itemActiveRight;

  // update map
  if (headCell) {
    state.GetWorld().GetMap().AddFeatureSeen(headCell->GetFeatureID());
  }

  // update messages
  auto iter = this->messages.begin();
  size_t messageNum = 0;
  while(iter!=this->messages.end() && messageNum++ < 2) {
    (*iter)->messageTime -= game.GetDeltaT();
    if ((*iter)->messageTime <= 0) {
      this->SetMessageY(this->GetMessageY() + (*iter)->text->GetSize().y);
      delete *iter;
      iter = this->messages.erase(iter);
    } else {
      iter++;
    }
  }
  this->bigMessageT -= game.GetDeltaT();
  if (this->bigMessageT < 0.0f) this->bigMessageT = 0.0f;

  this->SetMessageY(this->GetMessageY() + this->GetMessageVY() * game.GetDeltaT());
  this->SetMessageVY(this->GetMessageVY() - game.GetDeltaT() * 100);
  if (this->GetMessageY() < 0.0f) {
    this->SetMessageY(0.0f);
    this->SetMessageVY(0.0f);
  }
}

void
Player::UpdateInput(
  RunningState &state
) {
  Game &game = state.GetGame();
  float deltaT = game.GetDeltaT();
  Input &input = game.GetInput();

  if (input.IsKeyActive(InputKey::MapZoomIn))  this->mapZoom += deltaT * 10.0;
  if (input.IsKeyActive(InputKey::MapZoomOut)) this->mapZoom -= deltaT * 10.0;

  if (input.IsKeyDown(InputKey::DebugDie))     this->Die(state, HealthInfo());

  if (input.IsKeyDown(InputKey::Use) && !state.IsShowingInventory())
    this->UseItem(state, nullptr);

  this->SetSneaking(input.IsKeyActive(InputKey::Sneak));

  this->SetForward(Vector3(this->GetYaw(), this->GetPitch(), 0).EulerToVector());

  Vector3 fwd   = this->GetForward().Horiz().Normalize();
  Vector3 right = this->GetRight().Horiz().Normalize();
  float   speed = this->properties->maxSpeed * this->GetMoveModifier();

  move = Vector3();

  if (input.IsKeyActive(InputKey::Right))     move = move + right * speed;
  if (input.IsKeyActive(InputKey::Left))      move = move - right * speed;
  if (input.IsKeyActive(InputKey::Forward))   move = move + fwd   * speed;
  if (input.IsKeyActive(InputKey::Backward))  move = move - fwd   * speed;

  if (this->elements.size()) {
    if (input.IsKeyActive(InputKey::CastSpell)) {
      this->CastSpell(state);
    } else if (this->GetCastStartTime() != 0.0) {
      this->StopCasting();
    }
  }

  if (input.IsKeyActive(InputKey::Jump) && (this->IsOnGround() || this->IsInLiquid() || this->IsNoclip())) doesWantJump = true;
}

void Player::Draw(Gfx &gfx) const {
  (void)gfx;
}

void
Player::DrawWeapons(Gfx &gfx) const {
  gfx.SetShader("default");

  Vector3 fwd(0,0,1);
  Vector3 right(1,0,0);
  Vector3 bob = Vector3(0,sin(this->GetBobPhase() * 3.14159 * 4) * 0.05, 0) * this->GetBobAmplitude() + 
                  right * cos(this->GetBobPhase() * 3.14159 * 2) * 0.05 * this->GetBobAmplitude();

  Vector3 pos = Vector3(0,0,-1)+bob;

  gfx.GetView().Look(pos, fwd);
  gfx.SetColor(IColor(255,255,255));
  gfx.SetLight((this->cellLight + this->inventory.GetLight()).Saturate());
  if (this->inventory[InventorySlot::RightHand]) {
    this->inventory[InventorySlot::RightHand]->Draw(gfx, false);
  } else {
    this->rightHand->Draw(gfx, false);
  }
  if (this->inventory[InventorySlot::LeftHand]) {
    this->inventory[InventorySlot::LeftHand]->Draw(gfx, true);
  } else {
    this->leftHand->Draw(gfx, true);
  }
}

void
Player::DrawGUI(Gfx &gfx) const {
  gfx.SetShader("gui");
  gfx.SetColor(IColor(255,255,255));

  Point itemPos = gfx.GetScreen().AlignBottomRightScreen(Point(32,32), 4);
  gfx.SetTextureFrame(this->slotTex);
  gfx.DrawIconQuad(itemPos);
  gfx.DrawIconQuad(itemPos - Point(36,0));

  if (this->inventory[InventorySlot::RightHand]) {
    this->inventory[InventorySlot::RightHand]->DrawIcon(gfx, itemPos);
  } else {
    this->rightHand->DrawIcon(gfx, itemPos);
  }

  if (this->inventory[InventorySlot::LeftHand]) {
    this->inventory[InventorySlot::LeftHand]->DrawIcon(gfx, itemPos - Point(36,0));
  } else {
    this->leftHand->DrawIcon(gfx, itemPos - Point(36, 0));
  }

  size_t maxElements = GetEffectiveStats().GetSkill("magic");

  for (size_t i=0; i<maxElements; i++) {
    if (i >= this->elements.size()) {
      gfx.DrawIcon(gemSprites.at(Element::Physical), itemPos - Point(36,0) * (maxElements - i+1));
    } else {
      gfx.DrawIcon(gemSprites.at(elements[i]), itemPos - Point(36,0) * (maxElements - i+1));
    }
  }

  // draw crosshair
  const Point &vsize = gfx.GetScreen().GetVirtualSize();
  Sprite sprite;
  sprite.texture = crosshairTex;
  gfx.DrawIcon(sprite, Point(vsize.x/2, vsize.y/2));
  //gfx.DrawIcon(gemEmptySprite, Point(vsize.x/2, vsize.y/2));


  // draw messages
  float y = this->GetMessageY() + 4;
  size_t messageNum = 0;
  for (auto &msg : this->messages) {
    if (messageNum++ >= 2) break;
    float a = msg->messageTime * 4;
    if (a > 1.0) a = 1.0;
    gfx.SetColor(IColor(255,255,255), a);
    msg->text->WrapWords(vsize.x);
    msg->text->Draw(gfx, 4, y);
    y += msg->text->GetSize().y+2;
  }
  gfx.SetColor(IColor(255,255,255), this->bigMessageT);
  bigMessage->Draw(gfx, vsize/2 + Point(0,-100+this->bigMessageT*50), (int)(Align::HorizCenter | Align::VertMiddle));

  // draw health bar
  gfx.SetColor(IColor(255,255,255));
  std::string strHealth;
  int h = std::ceil(10 * this->GetHealth() / this->GetEffectiveStats().GetMaxHealth());
  std::string strHeart = u8"\u0081";
  if (h<4 && blink) strHeart = u8"\u0082";

  for (int i=0; i<11; i++) {
    if (i<h-1) strHealth += strHeart; else strHealth += " ";
  }
  RenderString rsHealth(strHealth + " " + ToString(int(this->GetHealth())), "big");
  gfx.SetColor(IColor(255, 255-255*this->GetHPFlashTime(), 255-255*this->GetHPFlashTime()));
  rsHealth.Draw(gfx, 2, vsize.y-4, (int)Align::HorizLeft | (int)Align::VertBottom);

  gfx.SetColor(IColor(255, 255, 255));

  char tmp[1024];
  Stats stats = this->GetEffectiveStats();
  /*
  snprintf(tmp, sizeof(tmp), "STR: %3d DEX: %3d AGI: %3d DEF: %3d MATK: %3d MDEF: %3d MAX HP: %3d",
    stats.GetStrength(), 
    stats.GetDexterity(), 
    stats.GetAgility(), 
    stats.GetDefense(), 
    stats.GetMagicAttack(), 
    stats.GetMagicDefense(), 
    stats.GetMaxHealth()
  );
  RenderString(tmp, "small").Draw(gfx, 4, vsize.y-32);
  */

  snprintf(tmp, sizeof(tmp), "LVL: %3u EXP: %4d / %4d", 
    stats.GetLevel(), 
    (int)stats.GetExperience(), 
    (int)Stats::GetExpForLevel(stats.GetLevel() + 1)
  );
  RenderString(tmp, "small").Draw(gfx, 4, vsize.y-32-12);

  std::string buffstring = stats.GetToolTip(true);
  for (auto &b:activeBuffs) {
    buffstring += "\n" + b.GetEffect().displayName;
  }
  RenderString(buffstring).Draw(gfx, vsize - Point(4,40), int(Align::HorizRight|Align::VertBottom));

  if (this->lookAtEntity) {
    RenderString(this->lookAtEntity->GetEffectiveStats().GetToolTip(true)).Draw(gfx, vsize/2, int(Align::HorizCenter));
  }

  std::string skills;
  for (auto &s:GetEffectiveStats().GetAllSkills()) {
    skills += s.first + ": " + ToString(Stats::GetLevelForSkillExp(s.second));
    skills += " (" + ToString(s.second) + ")\n";
  }

  snprintf(tmp, sizeof(tmp),
    u8"%s%3.1f\n"
    u8"\u0080 %-5u "
    u8"\u008B %-3u "
    u8"\u008C %-3u "
    u8"\u008D %-3u "
    u8"\u008E %-3u "
    u8"\u008F %-3u ",
    skills.c_str(), fps,
    this->GetGold(),
    this->GetGems(Element::Fire),
    this->GetGems(Element::Water),
    this->GetGems(Element::Wind),
    this->GetGems(Element::Earth),
    this->GetGems(Element::Life)
  );
  RenderString(tmp).Draw(gfx, 4, vsize.y-32-24, int(Align::VertBottom));
}

void
Player::HandleEvent(const InputEvent &event) {
  if (event.type == InputEventType::Key) {
    if (event.key == InputKey::MouseLeft)  this->itemActiveLeft  = event.down;
    if (event.key == InputKey::MouseRight) this->itemActiveRight = event.down;
    if (event.key == InputKey::DebugNoclip && event.down) this->SetNoClip(!this->IsNoclip());

    if (event.down) {
      if (event.key == InputKey::ElementClear) this->ClearElements();
      if (event.key == InputKey::ElementFire)  this->QueueElement(Element::Fire);
      if (event.key == InputKey::ElementWater) this->QueueElement(Element::Water);
      if (event.key == InputKey::ElementAir)   this->QueueElement(Element::Wind);
      if (event.key == InputKey::ElementEarth) this->QueueElement(Element::Earth);
      if (event.key == InputKey::ElementLife)  this->QueueElement(Element::Life);
    }
  } else if (event.type == InputEventType::MouseDelta) {
    float yaw = this->GetYaw();
    float pitch = this->GetPitch();
#if _WIN32
    yaw   +=  event.p.x*0.005;
    pitch += -event.p.y*0.005;
#elif MACOSX
    yaw   +=  event.p.x*0.005;
    pitch += -event.p.y*0.005;
#else
    yaw   +=  event.p.x*0.0005;
    pitch += -event.p.y*0.0005;
#endif

    if (pitch >  89_deg) pitch =  89_deg;
    if (pitch < -89_deg) pitch = -89_deg;

    this->SetYaw(yaw);
    this->SetPitch(pitch);
  }
}

void
Player::SetUniforms(const std::shared_ptr<Shader> &shader) const {
  if (shader) {
    shader->Uniform("u_fade", IColor(std::sqrt(this->GetPain())*255, 0, 0));
  }
}

void
Player::AddHealth(RunningState &state, const HealthInfo &info) {
  int hp = this->GetHealth();
  if (info.amount < 0) {
    this->SetPain(this->GetPain() - info.amount / this->properties->maxHealth);
  }

  Mob::AddHealth(state, info);
  int hp2 = this->GetHealth();
  if (hp2 < hp) {
    this->SetHPFlashTime(1.0f);
  }

  Entity *other = state.GetEntity(info.dealerId);
  if (other && other != this) {
    std::string otherName = other->GetName();
    if (otherName == "") {
      if (info.hitType == HitType::Miss) {
        this->AddMessage("Something missed you.");
      } else if (info.hitType == HitType::Normal) {
        this->AddMessage("You were hit for " + ToString(info.amount) + " hp.");
      } else if (info.hitType == HitType::Critical) {
        this->AddMessage("You were critically hit for " + ToString(info.amount) + " hp.");
      }
    } else {
      if (info.hitType == HitType::Miss) {
        this->AddMessage("The " + other->GetName() + " misses you.");
      } else if (info.hitType == HitType::Normal) {
        this->AddMessage("The " + other->GetName() + " hits you for " + ToString(info.amount) + " hp.");
      } else if (info.hitType == HitType::Critical) {
        this->AddMessage("The " + other->GetName() + " hits you critically for " + ToString(info.amount) + " hp.");
      }
    }
  }
}

void
Player::AddMessage(const std::string &text, const std::string &font) {
  Message *msg = new Message(text, font);
  if (this->messages.size() && this->messages.back()->messageTime > 1.8 && this->messages.back()->text->GetFontName() == font) {
    (*this->messages.back()->text) = this->messages.back()->text->GetText() + " " + text;
  } else {
    this->messages.push_back(msg);
  }
}

void
Player::AddDeathMessage(const Entity &dead, const HealthInfo &info) {
  if (dead.GetName() == "") return;

  if ((dead.GetPosition()-this->GetPosition()).GetMag() > 10) return;

  switch(info.type) {
    case HealthType::Unspecified: AddMessage(dead.GetName() + " died"); break;
    case HealthType::Heal:        AddMessage(dead.GetName() + " was unhealed"); break;
    case HealthType::Falling:     AddMessage(dead.GetName() + " killed the ground too quickly"); break;
    case HealthType::Explosion:   AddMessage(dead.GetName() + " blew up"); break;
    case HealthType::Melee:       AddMessage(dead.GetName() + " was killed"); break;
    case HealthType::Arrow:       AddMessage(dead.GetName() + " was deadly hit by an arrow"); break;
    case HealthType::Vampiric:    AddMessage(dead.GetName() + "'s blood was sucked"); break;
    case HealthType::Fire:        AddMessage(dead.GetName() + " burned "); break;
    case HealthType::Lava:        AddMessage(dead.GetName() + " tried to swim in lava"); break;
    case HealthType::Magic:       AddMessage(dead.GetName() + " saw the light"); break;
  }
}

void
Player::AddDeathMessage(const Entity &dead, const Entity &killer, const HealthInfo &info) {
  if (dead.GetName() == "") return;
  if ((dead.GetPosition()-this->GetPosition()).GetMag() > 10) return;

  switch(info.type) {
    case HealthType::Unspecified: AddMessage(dead.GetName() + " was killed by " + killer.GetName()); break;
    case HealthType::Heal:        AddMessage(dead.GetName() + " was unhealed by " + killer.GetName()); break;
    case HealthType::Falling:     AddMessage(dead.GetName() + " was doomed to fall by " + killer.GetName()); break;
    case HealthType::Explosion:   AddMessage(dead.GetName() + " was blown up by " + killer.GetName()); break;
    case HealthType::Melee:       AddMessage(dead.GetName() + " was smitten by " + killer.GetName()); break;
    case HealthType::Arrow:       AddMessage(dead.GetName() + " was deadly hit by " + killer.GetName() + "'s "); break;
    case HealthType::Vampiric:    AddMessage(dead.GetName() + "'s blood was sucked by " + killer.GetName()); break;
    case HealthType::Fire:        AddMessage(dead.GetName() + " was set on fire by " + killer.GetName()); break;
    case HealthType::Lava:        AddMessage(dead.GetName() + " was pushed into the lava by " + killer.GetName()); break;
    case HealthType::Magic:       AddMessage(dead.GetName() + " saw "+killer.GetName()+"'s light"); break;
  }
}

std::string
Player::GetName() const {
  return "Awesome player";
}

void
Player::OnHealthDealt(RunningState &state, Entity &other, const HealthInfo &info) {
  Mob::OnHealthDealt(state, other, info);

  if (other.GetName() == "") return;
  if (std::abs(info.amount) < 1.0) return;

  if (info.hitType == HitType::Miss) {
    this->AddMessage("You miss the " + other.GetName());
  } else if (info.hitType == HitType::Normal) {
    this->AddMessage("You hit the " + other.GetName() + " for " + ToString(int(info.amount)) + " hp");
  } else if (info.hitType == HitType::Critical) {
    this->AddMessage("You hit the " + other.GetName() + " critically for " + ToString(int(info.amount)) + " hp");
  }
}

void
Player::OnEquip(RunningState &state, const Item &item, InventorySlot slot, bool equip) {
  Mob::OnEquip(state, item, slot, equip);

  if (equip) {
    this->AddMessage("You put on the " + item.GetDisplayName() + ".");
    if (item.IsCursed()) {
      this->AddMessage("It is freezing cold.");
    }
    if (item.IsInited() && item.GetEffect().feeling != "")
      this->AddMessage("You feel "+item.GetEffect().feeling+".");
  } else {
    this->AddMessage("You take off the " + item.GetDisplayName() + ".");
    if (item.IsInited() && item.GetEffect().feeling != "")
      this->AddMessage("You no longer feel "+item.GetEffect().feeling+".");
  }
}

bool Player::UseItem(RunningState &state, const std::shared_ptr<Item> &item) {
  ID entityID;
  Side cellSide;
  // TODO: get default range
  Cell *cell = this->GetSelection(state, item ? item->GetProperties().range : 5.0, item, cellSide, entityID);

  if (item) {
    if (entityID != InvalidID) {
      return item->UseOnEntity(state, *this, entityID);
    } else if (cell) {
      return item->UseOnCell(state, *this, cell, cellSide);
    } else {
      return item->UseOnNothing(state, *this);
    }
  } else {
    if (entityID != InvalidID) {
      Entity *entity = state.GetEntity(entityID);
      if (entity) entity->OnUse(state, *this);
    } else if (cell) {
      cell->OnUse(state, *this);
    }
    return true;
  }
}

void Player::OnLevelUp(RunningState &) {
  // this->health ++;
  *this->bigMessage = "Level Up!";
  this->bigMessageT = 2.0;
}

void Player::OnBuffAdded(RunningState &, const EffectProperties &effect) {
  this->AddMessage("You feel "+effect.feeling);
}

void
Player::OnCollide(RunningState &state, Entity &other) {
  // auto pickup gold
  ItemEntity *itemEnt = dynamic_cast<ItemEntity*>(&other);
  if (itemEnt && itemEnt->GetItem()->GetType() == "gold") {
    itemEnt->OnUse(state, *this);
  }
}

void
Player::QueueElement(Element element) {
  size_t maxElements = GetEffectiveStats().GetSkill("magic");
  size_t elemCount = 1;
  for (Element e:this->elements) if (e == element) elemCount++;

  if (this->elements.size() < maxElements && elemCount <= this->GetGems(element)) {
    this->elements.push_back(element);
    this->PlaySound("gem."+ToString(this->elements.size()));
  } else {
    this->PlaySound("gem.full");
  }
}

void
Player::ClearElements() {
  this->PlaySound("gem.clear");
  this->elements.clear();
}

void
Player::CastSpell(RunningState &state) {
  // find spell from queued elements
  const Spell &spell = getSpell(this->elements);
  float t = state.GetGame().GetTime();

  if (this->GetCastStartTime() != 0.0 && t - this->GetCastStartTime() > spell.maxDuration) {
    this->StopCasting();
    return;
  } else if (t - this->GetLastCastTime() < spell.castInterval) return;


  Log("trying to cast spell %s\n", spell.name.c_str());

  if (spell.Cast(state, *this)) {
    if (this->GetCastStartTime() == 0.0) {
      AddMessage("You cast the spell "+spell.displayName);
      this->baseStats.UpgradeSkill("magic");
      this->SetCastStartTime(t);
    }
    this->SetLastCastTime(t);
  } else {
    AddMessage("Your spell fizzles.");
    this->elements.clear();
  }
}

void
Player::StopCasting() {
  for (Element e:this->elements)
    this->inventory.RemoveGem(e);
  this->elements.clear();
  this->SetCastStartTime(0.0f);
  this->SetLastCastTime(0.0f);
}

void
Player::LearnSpell(const std::string &name) {
  Log("%s %d\n", __PRETTY_FUNCTION__, __LINE__);
  Mob::LearnSpell(name);
  Log("%s %d\n", __PRETTY_FUNCTION__, __LINE__);
  AddMessage("You learn the spell "+getSpell(name).displayName);
  Log("%s %d\n", __PRETTY_FUNCTION__, __LINE__);
}

const Entity_Proto &
Player::GetProto() {

  this->proto.mutable_player()->clear_messages();
  for (auto &msg : this->messages) {
    PlayerMessage_Proto *p = this->proto.mutable_player()->add_messages();
    p->set_text(msg->text->GetText());
    p->set_font(msg->text->GetFontName());
    p->set_time(msg->messageTime);
  }

  this->proto.mutable_player()->clear_elements();
  for (auto &e : this->elements) {
    this->proto.mutable_player()->add_elements(uint32_t(e));
  }

  return Entity::GetProto();
}

Player::Message::Message(const std::string &txt, const std::string &font) :
  text(new RenderString(txt, font)),
  messageTime(2)
{}

Player::Message::~Message() {
  delete text;
}
