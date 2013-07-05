#include "player.h"
#include "world.h"
#include "cell.h"
#include "text.h"
#include "util.h"
#include "simplex.h"
#include "worldedit.h"
#include "gui.h"
#include "game.h"
#include "gfx.h"
#include "vertex.h"
#include "item.h"
#include "input.h"

static float eyeHeight = 0.7f;

Player::Player() : Mob("player") {
  aabb.center = Vector3(16,16,16);

  bobPhase = 0;
  bobAmplitude = 0;
  fps = 0;
  
  noclip = false;
  this->selectedCell = nullptr;
  this->selectedCellSide = Side::Forward;
  this->selectionRange = 0;
  this->selectedEntity = ~0UL;

  this->Equip(std::make_shared<Item>(Item("sword")), InventorySlot::RightHand);
  this->Equip(std::make_shared<Item>(Item("torch")), InventorySlot::LeftHand);
  this->AddToInventory(std::make_shared<Item>(Item("torch")));

  this->itemActiveLeft = false;
  this->itemActiveRight = false;

  this->crosshairTex = loadTexture("gui/crosshair");
}

Player::~Player() {
}

void
Player::View(Gfx &gfx) const {
  Vector3 fwd   = (GetAngles()).EulerToVector();
  Vector3 right = (GetAngles()+Vector3(3.14159/2, 0, 0)).EulerToVector();
  Vector3 bob = Vector3(0,sin(bobPhase*3.14159*4)*0.05, 0) * bobAmplitude + right * cos(bobPhase*3.14159*2)*0.05 * bobAmplitude;

  Vector3 pos = smoothPosition + Vector3(0,eyeHeight,0)+bob;
  
  gfx.View3D(pos, fwd);
}

void
Player::MapView(Gfx &gfx) const {
  Vector3 fwd = this->GetAngles().EulerToVector();
  Vector3 pos = this->smoothPosition + Vector3(0,16,0);
  
  gfx.View3D(pos, Vector3(0,-1,0), -32.0, fwd);
}

void 
Player::Update(Game &game) {
  Mob::Update(game);
  
  this->fps = game.GetFPS();

  UpdateInput(game);
  UpdateSelection(game);

  float t = game.GetTime();
  IColor torch;
  for (auto item : this->inventory) {
    if (!item || !item->IsEquipped()) continue;
    
    float f = 1.0;
    if (item->GetProperties()->flicker) {
      f = simplexNoise(Vector3(t*3, 0, 0)) * simplexNoise(Vector3(t*2, -t, 0));
      f = f * 0.4 + 0.5;
    }
    torch = torch + item->GetProperties()->light * f;
  }  
  this->torchLight = torch;
  
  if (itemActiveLeft && this->inventory[(size_t)InventorySlot::RightHand]) {
    if (this->inventory[(size_t)InventorySlot::RightHand]->GetRange() < this->selectionRange) {
      this->inventory[(int)InventorySlot::RightHand]->UseOnNothing(game, *this);
    } else if (this->selectedCell) {
      this->inventory[(int)InventorySlot::RightHand]->UseOnCell(game, *this, this->selectedCell, this->selectedCellSide);
    } else if (this->selectedEntity != ~0UL) {
      this->inventory[(int)InventorySlot::RightHand]->UseOnEntity(game, *this, this->selectedEntity);
    }
  }
  
  if (itemActiveRight && this->inventory[(size_t)InventorySlot::LeftHand]) {
    if (this->inventory[(size_t)InventorySlot::LeftHand]->GetRange() < this->selectionRange) {
      this->inventory[(int)InventorySlot::LeftHand]->UseOnNothing(game, *this);
    } else if (this->selectedCell) {
      this->inventory[(int)InventorySlot::LeftHand]->UseOnCell(game, *this, this->selectedCell, this->selectedCellSide);
    } else if (this->selectedEntity != ~0UL) {
      this->inventory[(int)InventorySlot::LeftHand]->UseOnEntity(game, *this, this->selectedEntity);
    }
  }
  
  game.GetWorld().SetTorchLight(this->torchLight);
  if (headCell) {
    game.GetWorld().AddFeatureSeen(headCell->GetFeatureID());
  }
}

void Player::UpdateSelection(Game &game) {
  // TODO: get from equipped items
  static const float range = 10.0;
  
  // update selection
  Vector3 dir = (this->GetAngles()).EulerToVector();
  Vector3 pos = this->smoothPosition + Vector3(0, eyeHeight, 0);
  
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

void Player::Draw(Gfx &gfx) const {
  (void)gfx;

  if (this->selectedCell) {
    std::vector<Vertex> verts;
    this->selectedCell->DrawHighlight(verts);
    gfx.SetTextureFrame(gfx.GetNoiseTexture());
    gfx.DrawTriangles(verts);
  }
}

void
Player::UpdateInput(
  Game &game
) {
  float deltaT = game.GetDeltaT();
  
  Input *input = game.GetInput();
  
  if (input->IsKeyActive(InputKey::DebugDie)) Die(game, HealthInfo());
  
  if (input->IsKeyDown(InputKey::Use) && this->selectedEntity != ~0UL) {
    temp_ptr<Entity> entity(game.GetEntity(this->selectedEntity));
    if (entity) entity->OnUse(game, *this);
  }

  sneak = input->IsKeyActive(InputKey::Sneak);

  if (angles.y > 3.1/2) angles.y = 3.1/2;
  if (angles.y < -3.1/2) angles.y = -3.1/2;

  Vector3 fwd( Vector3(angles.x, 0, 0).EulerToVector() );
  Vector3 right( Vector3(angles.x+3.14159/2, 0, 0).EulerToVector() );
  float speed = sneak?this->properties->maxSpeed*0.5:this->properties->maxSpeed;

  move = Vector3();
  
  if (input->IsKeyActive(InputKey::Right))     move = move + right * speed;
  if (input->IsKeyActive(InputKey::Left))      move = move - right * speed;
  if (input->IsKeyActive(InputKey::Forward))   move = move + fwd * speed;
  if (input->IsKeyActive(InputKey::Backward))  move = move - fwd * speed;

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
  bobPhase += deltaT*move.GetMag()/4;
  if (bobPhase >= 1.0) {
    bobPhase -= 1.0;
    // TODO: play step sound
  } else if (bobPhase >= 0.5 && lastPhase < 0.5) {
    // TODO: play step sound
  }
  
  if ((onGround || inWater || noclip) && input->IsKeyActive(InputKey::Jump)) wantJump = true;
}

void 
Player::DrawWeapons(Gfx &gfx) const {
  Vector3 fwd(0,0,1);
  Vector3 right(1,0,0);
  Vector3 bob = Vector3(0,sin(bobPhase*3.14159*4)*0.05, 0) * bobAmplitude + right * cos(bobPhase*3.14159*2)*0.05 * bobAmplitude;

  Vector3 pos = Vector3(0,0,-1)+bob;
  
  gfx.View3D(pos, fwd);

  IColor l = this->cellLight+this->torchLight;
  gfx.SetColor(l);

  if (this->inventory[(size_t)InventorySlot::RightHand]) {
    this->inventory[(size_t)InventorySlot::RightHand]->Draw(gfx, false);
  }
  if (this->inventory[(size_t)InventorySlot::LeftHand]) {
    this->inventory[(size_t)InventorySlot::LeftHand]->Draw(gfx, true);
  }
}

void 
Player::DrawGUI(Gfx &gfx) const {
  const Point &vsize = gfx.GetVirtualScreenSize();
  Sprite sprite;
  sprite.texture = crosshairTex;
  gfx.DrawIcon(sprite, Point(vsize.x/2, vsize.y/2));

  std::stringstream str;
  str << fps << std::endl;
  str << smoothPosition << std::endl;
  str << (this->selectedCell?this->selectedCell->GetType():"(null)") << std::endl;
  str << (this->headCell?this->headCell->GetFeatureID():~0UL) << std::endl;
  str << (this->selectedEntity) << std::endl;
  str << (this->selectionRange) << std::endl;
  RenderString rs(str.str());
  rs.Draw(gfx, 0,0);
}

void
Player::HandleEvent(const InputEvent &event) {
  if (event.type == InputEventType::Key) {
    if (event.key == InputKey::MouseLeft)  this->itemActiveLeft  = event.down;
    if (event.key == InputKey::MouseRight) this->itemActiveRight = event.down;
    if (event.key == InputKey::DebugNoclip && event.down) this->noclip = !this->noclip;
  } else if (event.type == InputEventType::MouseDelta) {
    angles.x += event.p.x*0.005;
    angles.y -= event.p.y*0.005;
  }
}

