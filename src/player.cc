#include "player.h"

#include "game.h"
#include "item.h"

#include "world.h"
#include "cell.h"

#include "gfx.h"
#include "shader.h"
#include "texture.h"
#include "text.h"
#include "vertex.h"

#include "input.h"

Player::Player() 
: Mob("player") 
{
  // rendering
  this->crosshairTex = loadTexture("gui/crosshair");
  this->defaultShader = std::unique_ptr<Shader>(new Shader("default"));
  this->guiShader = std::unique_ptr<Shader>(new Shader("gui"));
  
  this->bobPhase = 0;
  this->bobAmplitude = 0;
  
  // gameplay
  this->selectedCell = nullptr;
  this->selectedCellSide = Side::Forward;
  this->selectionRange = 0;
  this->selectedEntity = ~0UL;
  
  // display
  this->itemActiveLeft = false;
  this->itemActiveRight = false;

  this->messageY = 0;
  this->messageVY = 0;
  this->fps = 0;
  
  this->pain = 0.0;

  // TEST:
  this->inventory.Equip(std::make_shared<Item>(Item("sword")), InventorySlot::RightHand);
  this->inventory.Equip(std::make_shared<Item>(Item("torch")), InventorySlot::LeftHand);
  this->inventory.AddToBackpack(std::make_shared<Item>(Item("torch")));
  
}

Player::~Player() {
}

void
Player::View(Gfx &gfx) const {
  Vector3 fwd   = (GetAngles()).EulerToVector();
  Vector3 right = (GetAngles()+Vector3(3.14159/2, 0, 0)).EulerToVector();
  Vector3 bob = Vector3(0,sin(bobPhase*3.14159*4)*0.05, 0) * bobAmplitude + right * cos(bobPhase*3.14159*2)*0.05 * bobAmplitude;

  Vector3 pos = smoothPosition + Vector3(0,this->properties->eyeOffset,0)+bob;
  
  gfx.GetView().Look(pos, fwd);
}

void
Player::MapView(Gfx &gfx) const {
  Vector3 fwd = this->GetAngles().EulerToVector();
  Vector3 pos = this->smoothPosition + Vector3(0,16,0);
  
  gfx.GetView().Look(pos, Vector3(0,-1,0), -32.0, fwd);
}

void
Player::Start(Game &game, size_t id) {
  Mob::Start(game, id);
  
  this->pain = 0;
}

void 
Player::Update(Game &game) {
  Mob::Update(game);
  
  this->fps = game.GetFPS();
  this->pain -= game.GetDeltaT() * 0.1;
  if (this->pain < 0) this->pain = 0;

  UpdateInput(game);
  UpdateSelection(game);

  if (itemActiveLeft && this->inventory[InventorySlot::RightHand]) {
    if (this->inventory[InventorySlot::RightHand]->GetRange() < this->selectionRange) {
      this->inventory[InventorySlot::RightHand]->UseOnNothing(game, *this);
    } else if (this->selectedCell) {
      this->inventory[InventorySlot::RightHand]->UseOnCell(game, *this, this->selectedCell, this->selectedCellSide);
    } else if (this->selectedEntity != ~0UL) {
      this->inventory[InventorySlot::RightHand]->UseOnEntity(game, *this, this->selectedEntity);
    }
  }
  
  if (itemActiveRight && this->inventory[InventorySlot::LeftHand]) {
    if (this->inventory[InventorySlot::LeftHand]->GetRange() < this->selectionRange) {
      this->inventory[InventorySlot::LeftHand]->UseOnNothing(game, *this);
    } else if (this->selectedCell) {
      this->inventory[InventorySlot::LeftHand]->UseOnCell(game, *this, this->selectedCell, this->selectedCellSide);
    } else if (this->selectedEntity != ~0UL) {
      this->inventory[InventorySlot::LeftHand]->UseOnEntity(game, *this, this->selectedEntity);
    }
  }
  
  if (headCell) {
    game.GetWorld().AddFeatureSeen(headCell->GetFeatureID());
  }

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

  messageY += messageVY * game.GetDeltaT();
  messageVY -= game.GetDeltaT() * 100;
  if (messageY < 0) {
    messageY = 0;
    messageVY = 0;
  }
}

void Player::UpdateSelection(Game &game) {
  // TODO: get from equipped items
  static const float range = 10.0;
  
  // update selection
  Vector3 dir = (this->GetAngles()).EulerToVector();
  Vector3 pos = this->smoothPosition + Vector3(0, this->properties->eyeOffset, 0);
  
  float dist  = range;
  
  AABB aabbRange;
  aabbRange.center = pos;
  aabbRange.extents = Vector3(range,range,range); 
  auto entitiesInRange = game.FindEntities(aabbRange);

  float hitDist = range;
  Vector3 hitPos;

  this->selectedEntity = ~0UL;
  this->selectedCell = nullptr;
  
  // check entities in range  
  for (auto id : entitiesInRange) {
    temp_ptr<Entity> entity = game.GetEntity(id);
    if (!entity || entity == this) continue;
    if (entity->GetProperties()->nohit) continue;

    if (entity->GetAABB().Ray(pos, dir, hitDist, hitPos)) {
      if (hitDist < dist) { 
        dist = hitDist;
        this->selectedEntity = id;
      }
    }
  }
  
  // check cells
  Cell &cell = game.GetWorld().CastRayCell(pos, dir, hitDist, this->selectedCellSide);
  if (hitDist < dist) {
    dist = hitDist;
    this->selectedCell = &cell;
    this->selectedEntity = ~0UL;
  }
  
  this->selectionRange = dist;
}

void
Player::UpdateInput(
  Game &game
) {
  float deltaT = game.GetDeltaT();
  
  Input &input = game.GetInput();
  
  if (input.IsKeyDown(InputKey::DebugDie)) this->Die(game, HealthInfo());
  
  if (input.IsKeyDown(InputKey::Use) && this->selectedEntity != ~0UL) {
    temp_ptr<Entity> entity(game.GetEntity(this->selectedEntity));
    if (entity) entity->OnUse(game, *this);
  } else if (input.IsKeyDown(InputKey::Use) && this->selectedCell) {
    this->selectedCell->OnUse(game, *this);
  }

  sneak = input.IsKeyActive(InputKey::Sneak);

  if (angles.y >  3.1/2) angles.y =  3.1/2;
  if (angles.y < -3.1/2) angles.y = -3.1/2;

  Vector3 fwd( Vector3(angles.x, 0, 0).EulerToVector() );
  Vector3 right( Vector3(angles.x+3.14159/2, 0, 0).EulerToVector() );
  float speed = this->properties->maxSpeed * this->GetMoveModifier();

  move = Vector3();
  
  if (input.IsKeyActive(InputKey::Right))     move = move + right * speed;
  if (input.IsKeyActive(InputKey::Left))      move = move - right * speed;
  if (input.IsKeyActive(InputKey::Forward))   move = move + fwd * speed;
  if (input.IsKeyActive(InputKey::Backward))  move = move - fwd * speed;

  if (move.GetMag() > 1.5) {
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
    //this->AddMessage("step");
    //this->AddMessage(u8"\ufe000\ufe011\ufe022\ufe033\ufe044\ufe055\ufe066\ufe077\ufe088\ufe099\ufe0aa\ufe0bb\ufe0cc\ufe0dd\ufe0ee\ufe0ff", "big");
  } else if (bobPhase >= 0.5 && lastPhase < 0.5) {
    //this->AddMessage("step");
    //this->AddMessage(u8"\ufe000\ufe011\ufe022\ufe033\ufe044\ufe055\ufe066\ufe077\ufe088\ufe099\ufe0aa\ufe0bb\ufe0cc\ufe0dd\ufe0ee\ufe0ff");
    // TODO: play step sound
  }
  
  if ((onGround || inWater || noclip) && input.IsKeyActive(InputKey::Jump)) wantJump = true;
}

void Player::Draw(Gfx &gfx) const {
  gfx.SetShader(this->defaultShader.get());

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
  gfx.SetShader(this->defaultShader.get());
  
  Vector3 fwd(0,0,1);
  Vector3 right(1,0,0);
  Vector3 bob = Vector3(0,sin(bobPhase*3.14159*4)*0.05, 0) * bobAmplitude + right * cos(bobPhase*3.14159*2)*0.05 * bobAmplitude;

  Vector3 pos = Vector3(0,0,-1)+bob;
  
  gfx.GetView().Look(pos, fwd);
  gfx.SetColor(this->GetLight() + this->cellLight);

  if (this->inventory[InventorySlot::RightHand]) {
    this->inventory[InventorySlot::RightHand]->Draw(gfx, false);
  }
  if (this->inventory[InventorySlot::LeftHand]) {
    this->inventory[InventorySlot::LeftHand]->Draw(gfx, true);
  }
}

void 
Player::DrawGUI(Gfx &gfx) const {
  gfx.SetShader(this->guiShader.get());
  gfx.SetColor(IColor(255,255,255));
  
  const Point &vsize = gfx.GetVirtualScreenSize();
  Sprite sprite;
  sprite.texture = crosshairTex;
  gfx.DrawIcon(sprite, Point(vsize.x/2, vsize.y/2));

  float y = this->messageY;
  for (auto &msg : this->messages) {
    float a = msg->messageTime * 4;
    if (a > 1.0) a = 1.0;
    gfx.SetColor(IColor(255,255,255), a);
    msg->text->WrapWords(vsize.x);
    msg->text->Draw(gfx, 0, y);
    y += msg->text->GetSize().y;
  }
  
  std::stringstream strHealth;
  int h = this->health * 2;
  for (int i=0; i<h; i++) {
    if (i == h-1 && (h%2))
      strHealth << u8"\u0082";
    else
      strHealth << u8"\u0081";
  }
  RenderString rsHealth(strHealth.str());
  rsHealth.Draw(gfx, 0+4, vsize.y-4, (int)Align::Left | (int)Align::Bottom);
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
#else
    angles.x += event.p.x*0.0005;
    angles.y -= event.p.y*0.0005;
#endif
  }
}

void Player::SetUniforms(const Shader *shader) const {
  shader->Uniform("u_fade", IColor(std::sqrt(this->pain)*255, 0, 0));
}

void 
Player::AddHealth(Game &game, const HealthInfo &info) {
  if (info.amount < 0) {
    if (!IsContinuous(info.type) || game.GetTime() > lastHurtT[info.type] + 0.25) {
      this->AddMessage("ouch!");
      lastHurtT[info.type] = game.GetTime();
    }
    this->pain -= info.amount / this->properties->maxHealth;
    std::cerr << this->pain << std::endl;
  }
  
  Mob::AddHealth(game, info);
}

void 
Player::AddMessage(const std::string &text, const std::string &font) {
  Message *msg = new Message(text, font);
  msg->messageTime = 5;
  this->messages.push_back(msg);
}

void 
Player::AddDeathMessage(const Entity &dead, const HealthInfo &info) {
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
  return "<insert player name here>";
}


Player::Message::Message(const std::string &txt, const std::string &font) {
  this->text = new RenderString(txt, font);
}

Player::Message::~Message() {
  delete text;
}

