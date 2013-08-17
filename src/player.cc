#include "player.h"

#include "runningstate.h"
#include "item.h"

#include "world.h"
#include "cell.h"

#include "gfx.h"
#include "gfxview.h"
#include "shader.h"
#include "texture.h"
#include "text.h"
#include "vertex.h"

#include "input.h"

#include "serializer.h"
#include "deserializer.h"

#include <sstream>

Player::Player() : 
  Mob("player"),
  
  // rendering
  crosshairTex      (loadTexture("gui/crosshair")),
  slotTex           (loadTexture("gui/slot")),
  
  bobPhase          (0.0),
  bobAmplitude      (0.0),
  
  // gameplay
  selectedEntity    (InvalidID),
  selectedCell      (nullptr),
  selectedCellSide  (Side::Forward),
  selectionRange    (0.0),

  itemActiveLeft    (false),
  itemActiveRight   (false),
  
  lastHurtT         (),
  pain              (0.0),

  // display
  messages          (),
  messageY          (0.0),
  messageVY         (0.0),
  
  fps               (0.0),
  
  bigMessage        (new RenderString("", "big")),
  bigMessageT       (0.0),
  
  mapZoom           (-32.0f),
  
  leftHand          (new Item("barehand.player")),
  rightHand         (new Item("barehand.player"))
{
  // TEST:
  this->inventory.Equip(std::make_shared<Item>(Item("sword")), InventorySlot::RightHand);
  this->inventory.Equip(std::make_shared<Item>(Item("torch")), InventorySlot::LeftHand);
  this->inventory.AddToBackpack(std::make_shared<Item>(Item("torch")));
  
}

Player::Player(Deserializer &deser) : Mob("player", deser),
  // rendering
  crosshairTex      (loadTexture("gui/crosshair")),
  slotTex           (loadTexture("gui/slot")),
  
  bobPhase          (0.0),
  bobAmplitude      (0.0),
  
  // gameplay
  selectedEntity    (InvalidID),
  selectedCell      (nullptr),
  selectedCellSide  (Side::Forward),
  selectionRange    (0.0),

  itemActiveLeft    (false),
  itemActiveRight   (false),
  lastHurtT         (),
  pain              (0.0),

  // display
  fps               (0.0),
  
  bigMessage        (new RenderString("", "big")),
  bigMessageT       (0.0),
  
  leftHand          (new Item("barehand.player")),
  rightHand         (new Item("barehand.player"))
{
  deser >> pain;
  deser >> messages >> messageY >> messageVY;
  deser >> lastHurtT;
}

Player::~Player() {
  delete this->bigMessage;
}

void
Player::View(Gfx &gfx) const {
  Vector3 fwd   = this->GetForward();
  Vector3 right = this->GetRight();
  Vector3 bob   = Vector3(0,1,0) * std::abs(std::sin(bobPhase * Const::pi * 2)*0.05) * bobAmplitude + 
                  right          *          std::cos(bobPhase * Const::pi * 2)*0.05  * bobAmplitude;
  
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
  this->pain = 0;
}

void 
Player::Update(RunningState &state) {
  Mob::Update(state);
  
  Game &game = state.GetGame();
  
  this->fps = game.GetFPS();
  
  this->pain -= game.GetDeltaT() * 0.1;
  if (this->pain < 0) this->pain = 0;

  this->UpdateInput(state);
  this->UpdateSelection(state);
  
  this->rightHand->Update(state);
  this->leftHand->Update(state);
  
  if (itemActiveLeft) {
    std::shared_ptr<Item> useItem = this->inventory[InventorySlot::RightHand];
    if (!useItem) useItem = this->rightHand;
  
    if (useItem->GetRange() < this->selectionRange) {
      useItem->UseOnNothing(state, *this);
    } else if (this->selectedCell) {
      useItem->UseOnCell(state, *this, this->selectedCell, this->selectedCellSide);
    } else if (this->selectedEntity != InvalidID) {
      useItem->UseOnEntity(state, *this, this->selectedEntity);
    }
  }
  
  if (itemActiveRight) {
    std::shared_ptr<Item> useItem = this->inventory[InventorySlot::LeftHand];
    if (!useItem) useItem = this->leftHand;
  
    if (useItem->GetRange() < this->selectionRange) {
      useItem->UseOnNothing(state, *this);
    } else if (this->selectedCell) {
      useItem->UseOnCell(state, *this, this->selectedCell, this->selectedCellSide);
    } else if (this->selectedEntity != InvalidID) {
      useItem->UseOnEntity(state, *this, this->selectedEntity);
    }
  }
  
  // update map
  if (headCell) {
    state.GetWorld().GetMap().AddFeatureSeen(headCell->GetFeatureID());
  }

  // update messages
  auto iter = this->messages.begin();
  while(iter!=this->messages.end()) {
    (*iter)->messageTime -= game.GetDeltaT();
    if ((*iter)->messageTime <= 0) {
      this->messageY += (*iter)->text->GetSize().y;
      delete *iter;
      iter = this->messages.erase(iter);
    } else {
      iter++;
    }
  }
  this->bigMessageT -= game.GetDeltaT();
  if (this->bigMessageT < 0.0f) this->bigMessageT = 0.0f;

  messageY += messageVY * game.GetDeltaT();
  messageVY -= game.GetDeltaT() * 100;
  if (messageY < 0) {
    messageY = 0;
    messageVY = 0;
  }
}

void Player::UpdateSelection(RunningState &state) {
  // TODO: get from equipped items
  static const float range = 10.0;
  
  // update selection
  Vector3 dir = (this->GetAngles()).EulerToVector();
  Vector3 pos = this->smoothPosition + Vector3(0, this->properties->eyeOffset, 0);
  
  float dist  = range;
  
  AABB aabbRange;
  aabbRange.center = pos;
  aabbRange.extents = Vector3(range,range,range); 
  auto entitiesInRange = state.FindEntities(aabbRange);

  float hitDist = range;
  Vector3 hitPos;

  this->selectedEntity = InvalidID;
  this->selectedCell = nullptr;
  
  // check entities in range  
  for (auto id : entitiesInRange) {
    Entity *entity = state.GetEntity(id);
    if (!entity || entity == this) continue;
    if (entity->IsDead()) continue;
    if (entity->GetProperties()->nohit) continue;

    if (entity->GetAABB().Ray(pos, dir, hitDist, hitPos)) {
      if (hitDist < dist) { 
        dist = hitDist;
        this->selectedEntity = id;
      }
    }
  }
  
  // check cells
  Cell &cell = state.GetWorld().CastRayCell(pos, dir, hitDist, this->selectedCellSide);
  if (hitDist < dist) {
    dist = hitDist;
    this->selectedCell = &cell;
    this->selectedEntity = InvalidID;
  }
  
  this->selectionRange = dist;
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
  
  if (!state.IsShowingInventory()) {
    if (input.IsKeyDown(InputKey::Use) && this->selectedEntity != InvalidID) {
      Entity *entity = state.GetEntity(this->selectedEntity);
      if (entity) entity->OnUse(state, *this);
    } else if (input.IsKeyDown(InputKey::Use) && this->selectedCell) {
      this->selectedCell->OnUse(state, *this);
    }
  }
  sneak = input.IsKeyActive(InputKey::Sneak);

  if (angles.y >  89_deg) angles.y =  89_deg;
  if (angles.y < -89_deg) angles.y = -89_deg;

  Vector3 fwd   = this->GetForward().Horiz().Normalize();
  Vector3 right = this->GetRight().Horiz().Normalize();
  float   speed = this->properties->maxSpeed * this->GetMoveModifier();

  move = Vector3();
  
  if (input.IsKeyActive(InputKey::Right))     move = move + right * speed;
  if (input.IsKeyActive(InputKey::Left))      move = move - right * speed;
  if (input.IsKeyActive(InputKey::Forward))   move = move + fwd   * speed;
  if (input.IsKeyActive(InputKey::Backward))  move = move - fwd   * speed;

  if (!this->inWater && move.GetMag() > 1.5) {
    bobAmplitude += deltaT*4;
    if (bobAmplitude > 1.0) bobAmplitude = 1.0;
  } else {
    bobAmplitude -= deltaT*4;
    if (bobAmplitude < 0.0) {
      bobAmplitude = 0.0;
      bobPhase = 0;
    }
  }

  float lastPhase = bobPhase;
  bobPhase += (deltaT * this->GetMoveModifier()) * move.GetMag()/4;
  if (bobPhase >= 1.0) {
    bobPhase -= 1.0;
    // TODO: play step sound
  } else if (bobPhase >= 0.5 && lastPhase < 0.5) {
    // TODO: play step sound
  }
  
  if ((onGround || inWater || noclip) && input.IsKeyActive(InputKey::Jump)) wantJump = true;
}

void Player::Draw(Gfx &gfx) const {
  gfx.SetShader("default");

  if (this->selectedCell) {
    std::vector<Vertex> verts;
    this->selectedCell->DrawHighlight(verts);
    gfx.SetTextureFrame(loadTexture("cells/texture/select"));
    gfx.SetColor(IColor(255,255,255), 0.5);
    gfx.DrawTriangles(verts);
  }
}

void 
Player::DrawWeapons(Gfx &gfx) const {
  gfx.SetShader("default");
  
  Vector3 fwd(0,0,1);
  Vector3 right(1,0,0);
  Vector3 bob = Vector3(0,sin(bobPhase*3.14159*4)*0.05, 0) * bobAmplitude + right * cos(bobPhase*3.14159*2)*0.05 * bobAmplitude;

  Vector3 pos = Vector3(0,0,-1)+bob;
  
  gfx.GetView().Look(pos, fwd);
  gfx.SetColor(IColor(255,255,255));
  gfx.SetLight(this->cellLight + this->inventory.GetLight());

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
  
  Point itemPos = gfx.AlignBottomRightScreen(Point(32,32), 4);
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
  
  // draw crosshair
  const Point &vsize = gfx.GetVirtualScreenSize();
  Sprite sprite;
  sprite.texture = crosshairTex;
  gfx.DrawIcon(sprite, Point(vsize.x/2, vsize.y/2));

  // draw messages
  float y = this->messageY + 4;
  for (auto &msg : this->messages) {
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
  int h = 10 * this->health / this->GetEffectiveStats().maxHealth;
  for (int i=0; i<10; i++) {
    if (i < h)
      strHealth += u8"\u0081";
    else
      strHealth += u8"\u0082";
  }
  RenderString rsHealth(strHealth + " " + ToString(int(this->health)), "big");
  rsHealth.Draw(gfx, 2, vsize.y-4, (int)Align::HorizLeft | (int)Align::VertBottom);
  
  char tmp[1024];
  Stats stats = this->GetEffectiveStats();
  snprintf(tmp, sizeof(tmp), "STR: %3d DEX: %3d AGI: %3d DEF: %3d MAX HP: %3d", 
    stats.str, stats.dex, stats.agi, stats.def, stats.maxHealth
  );
  RenderString(tmp, "small").Draw(gfx, 4, vsize.y-32);

  snprintf(tmp, sizeof(tmp), "LVL: %3u EXP: %4d / %4d", (unsigned int)stats.GetLevel(), (int)stats.exp, (int)Stats::GetExpForLevel(stats.GetLevel() + 1));
  RenderString(tmp, "small").Draw(gfx, 4, vsize.y-32-12);

  std::string buffstring;
  for (auto &b:activeBuffs) {
    buffstring += "\n" + b.effect->displayName;
  }
  RenderString(buffstring).Draw(gfx, vsize - Point(4,40), int(Align::HorizRight|Align::VertBottom));

  snprintf(tmp, sizeof(tmp), "%3.1f", fps);
  RenderString(tmp, "small").Draw(gfx, 4, vsize.y-32-24);
}

void
Player::HandleEvent(const InputEvent &event) {
  if (event.type == InputEventType::Key) {
    if (event.key == InputKey::MouseLeft)  this->itemActiveLeft  = event.down;
    if (event.key == InputKey::MouseRight) this->itemActiveRight = event.down;
    if (event.key == InputKey::DebugNoclip && event.down) this->noclip = !this->noclip;
  } else if (event.type == InputEventType::MouseDelta) {
#if WIN32
    angles.x += event.p.x*0.005;
    angles.y -= event.p.y*0.005;
#elif MACOSX
    angles.x += event.p.x*0.005;
    angles.y -= event.p.y*0.005;
#else
    angles.x += event.p.x*0.0005;
    angles.y -= event.p.y*0.0005;
#endif
  }
}

void Player::SetUniforms(const std::shared_ptr<Shader> &shader) const {
  shader->Uniform("u_fade", IColor(std::sqrt(this->pain)*255, 0, 0));
}

void 
Player::AddHealth(RunningState &state, const HealthInfo &info) {
  Game &game = state.GetGame();
  
  int hp = this->health;
  if (info.amount < 0) {
    //if (!IsContinuous(info.type) || game.GetTime() > lastHurtT[(size_t)info.type] + 0.25) {
    //}
    this->pain -= info.amount / this->properties->maxHealth;
  }
  
  Mob::AddHealth(state, info);
  int hp2 = this->health;
  if (hp2 < hp) {
    this->AddMessage("Ouch!");
    lastHurtT[(size_t)info.type] = game.GetTime();
  }
  
  Entity *other = state.GetEntity(info.dealerId);
  if (other && other != this) {
    if (info.hitType == HitType::Miss) {
      this->AddMessage("The " + other->GetName() + " misses");
    } else if (info.hitType == HitType::Normal) {
      this->AddMessage("The " + other->GetName() + " hits you for " + ToString(info.amount) + " hp");
    } else if (info.hitType == HitType::Critical) {
      this->AddMessage("The " + other->GetName() + " hits you critically for " + ToString(info.amount) + " hp"); 
    }
  }
}

void 
Player::AddMessage(const std::string &text, const std::string &font) {
  Message *msg = new Message(text, font);
  this->messages.push_back(msg);
}

void 
Player::AddDeathMessage(const Entity &dead, const HealthInfo &info) {
  if (dead.GetName() == "") return;
  
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
  }
}

void 
Player::AddDeathMessage(const Entity &dead, const Entity &killer, const HealthInfo &info) {
  if (dead.GetName() == "") return;
  
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
  } else {
    this->AddMessage("You take off the " + item.GetDisplayName() + ".");
  }
}

void Player::OnLevelUp(RunningState &) {
  this->health ++;
  *this->bigMessage = "Level Up!";
  this->bigMessageT = 2.0;
}

void Player::OnBuffAdded(RunningState &, const EffectProperties &effect) {
  this->AddMessage("You feel "+effect.feeling);
}

void 
Player::Serialize(Serializer &ser) const {
  Mob::Serialize(ser);
  
  ser << pain;
  ser << messages << messageY << messageVY;
  ser << lastHurtT;
}

Player::Message::Message(const std::string &txt, const std::string &font) :
  text(new RenderString(txt, font)),
  messageTime(5)
{}

Player::Message::~Message() {
  delete text;
}

Serializer &operator << (Serializer &ser, const Player::Message *msg) {
  return ser << msg->text->GetText() << msg->text->GetFontName() << msg->messageTime;
}

Deserializer &operator >> (Deserializer &deser, Player::Message *&msg) {
  std::string str, font;
  float t;
  deser >> str >> font >> t;
  
  msg = new Player::Message(str, font);
  msg->messageTime = t;
  
  return deser;
  
}
