#include "player.h"
#include "world.h"
#include "cell.h"
#include "text.h"
#include "util.h"
#include "simplex.h"
#include "worldedit.h"
#include "weapon.h"
#include "gui.h"
#include "game.h"

#include <GL/glfw.h>
#include <cmath>

static float eyeHeight = 0.7f;

extern float mouseDX, mouseDY;
extern int screenWidth, screenHeight;

Player::Player() : Mob("player") {
  aabb.center = Vector3(16,16,16);

  bobPhase = 0;
  bobAmplitude = 0;
  
  noclip = false;
  this->selectedCell = nullptr;

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
Player::View() const {
  Vector3 fwd   = (GetAngles()).EulerToVector();
  Vector3 right = (GetAngles()+Vector3(3.14159/2, 0, 0)).EulerToVector();
  Vector3 bob = Vector3(0,sin(bobPhase*3.14159*4)*0.05, 0) * bobAmplitude + right * cos(bobPhase*3.14159*2)*0.05 * bobAmplitude;

  Vector3 pos = smoothPosition + Vector3(0,eyeHeight,0)+bob;
  Vector3 tpos = pos + fwd;

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  gluLookAt(pos.x, pos.y, pos.z, tpos.x, tpos.y, tpos.z, 0,1,0);
}

void
Player::MapView() const {
  if (headCell) {
    Game::Instance->GetWorld()->AddFeatureSeen(headCell->GetFeatureID());
  }

  Vector3 fwd   = (GetAngles()).EulerToVector();
  Vector3 pos(smoothPosition);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(-32, 32, -32, 32, 0.15, 64);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  gluLookAt(pos.x, pos.y+16, pos.z, pos.x, 0, pos.z, fwd.x,fwd.y,fwd.z);
}

void 
Player::Update() {
  Mob::Update();

  UpdateInput();
  UpdateSelection();

  if (itemActiveLeft && this->inventory[(size_t)InventorySlot::RightHand] &&
      this->inventory[(size_t)InventorySlot::RightHand]->GetRange() >= this->selectionRange) {
    if (this->selectedCell) {
      this->inventory[(int)InventorySlot::RightHand]->UseOnCell(this->selectedCell, this->selectedCellSide);
    } else {
      this->inventory[(int)InventorySlot::RightHand]->UseOnEntity(this->selectedEntity);
    }
  }
  
  if (itemActiveRight && this->inventory[(size_t)InventorySlot::LeftHand] &&
      this->inventory[(size_t)InventorySlot::LeftHand]->GetRange() >= this->selectionRange) {
    if (this->selectedCell) {
      this->inventory[(int)InventorySlot::LeftHand]->UseOnCell(this->selectedCell, this->selectedCellSide);
    } else {
      this->inventory[(int)InventorySlot::LeftHand]->UseOnEntity(this->selectedEntity);
    }
  }
  
  Game::Instance->GetWorld()->SetTorchLight(this->GetTorchLight());
}

void Player::UpdateSelection() {
  static const float range = 10.0;
  
  // update selection
  Vector3 dir = (this->GetAngles()).EulerToVector();
  Vector3 pos = this->smoothPosition + Vector3(0, eyeHeight, 0);
  
  float dist  = range;
  
  AABB aabbRange;
  aabbRange.center = pos;
  aabbRange.extents = Vector3(range,range,range); 
  auto entitiesInRange = Game::Instance->FindEntities(aabbRange);

  float hitDist = range;
  Vector3 hitPos;

  this->selectedEntity = ~0UL;
  this->selectedCell = nullptr;
  
  // check entities in range  
  for (auto id : entitiesInRange) {
    temp_ptr<Entity> entity = Game::Instance->GetEntity(id);
    if (!entity || entity == this) continue;

    if (entity->GetAABB().Ray(pos, dir, hitDist, hitPos)) {
      if (hitDist < dist) { 
        dist = hitDist;
        this->selectedEntity = id;
      }
    }
  }
  
  // check cells
  Cell &cell = Game::Instance->GetWorld()->CastRayCell(pos, dir, hitDist, this->selectedCellSide);
  if (hitDist < dist) {
    dist = hitDist;
    this->selectedCell = &cell;
    this->selectedEntity = ~0UL;
  }
  
  this->selectionRange = dist;
}

void Player::Draw() const {
  //Vector3 pos(this->spawnPos);
  //pos.y = this->smoothPosition.y;
  //glColor3ub(255, 255, 255);
  //activeItem->DrawBillboard(pos);
}

void
Player::UpdateInput(
) {
  float deltaT = Game::Instance->GetDeltaT();
  angles.x += mouseDX*0.5;
  angles.y -= mouseDY*0.5;
  
  mouseDX = mouseDY = 0;
  
  if (glfwGetKey('R')) Die();
  if (glfwGetKey('E') && this->selectedEntity != ~0UL) {
    temp_ptr<Entity> entity(Game::Instance->GetEntity(this->selectedEntity));
    if (entity) entity->OnUse(*this);
  }

  if (glfwGetKey(GLFW_KEY_LEFT))  angles.x -= deltaT;
  if (glfwGetKey(GLFW_KEY_RIGHT)) angles.x += deltaT;
  if (glfwGetKey(GLFW_KEY_DOWN))  angles.y -= deltaT;
  if (glfwGetKey(GLFW_KEY_UP))    angles.y += deltaT;
  
  sneak = glfwGetKey(GLFW_KEY_LSHIFT);

  if (angles.y > 3.1/2) angles.y = 3.1/2;
  if (angles.y < -3.1/2) angles.y = -3.1/2;

  Vector3 fwd( Vector3(angles.x, 0, 0).EulerToVector() );
  Vector3 right( Vector3(angles.x+3.14159/2, 0, 0).EulerToVector() );
  float speed = sneak?this->properties->maxSpeed*0.5:this->properties->maxSpeed;

  move = Vector3();
  
  if (glfwGetKey('D')) move = move + right * speed;
  if (glfwGetKey('A')) move = move - right * speed;
  if (glfwGetKey('W')) move = move + fwd * speed;
  if (glfwGetKey('S')) move = move - fwd * speed;

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
  
  if ((onGround || inWater || noclip) && glfwGetKey(' ')) wantJump = true;
}

void 
Player::DrawWeapons() const {
  Vector3 fwd(0,0,1);
  Vector3 right(1,0,0);
  Vector3 bob = Vector3(0,sin(bobPhase*3.14159*4)*0.05, 0) * bobAmplitude + right * cos(bobPhase*3.14159*2)*0.05 * bobAmplitude;

  Vector3 pos = Vector3(0,0,-1)+bob;
  Vector3 tpos = pos + fwd;
  
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  gluLookAt(pos.x, pos.y, pos.z, tpos.x, tpos.y, tpos.z, 0,1,0);

  IColor l = light+GetTorchLight();
  glColor3f(l.r/255.0, l.g/255.0, l.b/255.0);

  if (this->inventory[(size_t)InventorySlot::RightHand]) {
    this->inventory[(size_t)InventorySlot::RightHand]->Draw(false);
  }
  if (this->inventory[(size_t)InventorySlot::LeftHand]) {
    this->inventory[(size_t)InventorySlot::LeftHand]->Draw(true);
  }
}

void 
Player::DrawGUI() const {
  viewGUI();
  drawIcon(Point(virtualScreenWidth/2, virtualScreenHeight/2), Point(32,32), crosshairTex);  
}

void
Player::MouseClick(const Point &pos, int button, bool down) {
  (void)pos;
  if (button == GLFW_MOUSE_BUTTON_LEFT) this->itemActiveLeft = down;
  if (button == GLFW_MOUSE_BUTTON_RIGHT) this->itemActiveRight = down;
}

const IColor Player::GetTorchLight() const {
  IColor torch;
  float t = Game::Instance->GetTime();
  
  for (auto item : this->inventory) {
    if (!item || !item->IsEquipped()) continue;
    
    float f = 1.0;
    if (item->GetProperties()->flicker) {
      f = simplexNoise(Vector3(t*3, 0, 0)) * simplexNoise(Vector3(t*2, -t, 0));
      f = f * 0.4 + 0.5;
    }
    torch = torch + item->GetProperties()->light * f;
  }
  return torch;
}


