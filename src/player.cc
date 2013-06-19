#include "player.h"
#include "world.h"
#include "cell.h"
#include "text.h"
#include "util.h"
#include "simplex.h"
#include "worldedit.h"
#include "weapon.h"

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
  
  this->inventory[(size_t)InventorySlot::RightHand] = std::make_shared<Weapon>(Weapon());
  this->inventory[(size_t)InventorySlot::LeftHand] = std::make_shared<Weapon>(Weapon());

  this->itemActiveLeft = false;
  this->itemActiveRight = false;

  this->crosshairTex = loadTexture("gui/crosshair");
  this->slotTex = loadTexture("gui/slot");
  this->slotLeftHandTex = loadTexture("gui/slot-lh");
  this->slotRightHandTex = loadTexture("gui/slot-rh");
}

Player::~Player() {
}

void
Player::View() {
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
Player::MapView() {
  if (!headCell) return;

  this->world->AddFeatureSeen(headCell->GetFeatureID());

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
Player::Update(float t) {
  Mob::Update(t);
  if (!this->world) return;

  UpdateInput();

  Vector3 fwd   = (GetAngles()).EulerToVector();
  Vector3 pos = smoothPosition + Vector3(0,eyeHeight,0);
  if (itemActiveLeft && this->inventory[(size_t)InventorySlot::RightHand]) {
    this->inventory[(int)InventorySlot::RightHand]->Use(*this, pos, fwd, true);
  }
  if (itemActiveRight && this->inventory[(size_t)InventorySlot::LeftHand]) {
    this->inventory[(int)InventorySlot::LeftHand]->Use(*this, pos, fwd, true);
  }
  
  //float torch = simplexNoise(Vector3(lastT*4, lastT, 0)) * simplexNoise(Vector3(lastT*5, -lastT, 0));
  //this->torchLight = IColor(torch*128+192, (torch*128+192)*0.9, (torch*128+192)*0.6);
  this->torchLight = IColor(32,32,128);
}

void Player::Draw() {
  //Vector3 pos(this->spawnPos);
  //pos.y = this->smoothPosition.y;
  //glColor3ub(255, 255, 255);
  //activeItem->DrawBillboard(pos);
}

void
Player::UpdateInput(
) {
  angles.x += mouseDX*0.5;
  angles.y -= mouseDY*0.5;
  
  if (glfwGetKey('R')) Die();

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
Player::DrawWeapons() {
  Vector3 fwd(0,0,1);
  Vector3 right(1,0,0);
  Vector3 bob = Vector3(0,sin(bobPhase*3.14159*4)*0.05, 0) * bobAmplitude + right * cos(bobPhase*3.14159*2)*0.05 * bobAmplitude;

  Vector3 pos = Vector3(0,0,-1)+bob;
  Vector3 tpos = pos + fwd;
  
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  gluLookAt(pos.x, pos.y, pos.z, tpos.x, tpos.y, tpos.z, 0,1,0);

  IColor l = light + torchLight;
  glColor3f(l.r/255.0, l.g/255.0, l.b/255.0);

  this->inventory[(size_t)InventorySlot::RightHand]->Draw(false);
  this->inventory[(size_t)InventorySlot::LeftHand]->Draw(true);
}

void 
Player::DrawGUI() {
  viewGUI();
  
  glColor3ub(255, 255, 255);
  
  drawIcon(screenWidth/2, screenHeight/2, 16, 16, crosshairTex);  
  
  float scale = 1;
  if (screenWidth <= 640) scale = 1;
  else scale = 2;
  
//  drawIcon(screenWidth-16*scale, screenHeight-(32+16)*scale, 16*scale, 16*scale, slotTex);
//  inventory[(size_t)InventorySlot::RightHand]->DrawIcon(screenWidth-16*scale, screenHeight-(32+16)*scale, 16*scale, 16*scale);

  DrawInventorySlot(screenWidth - 16*scale, screenHeight-(32+16)*scale, (size_t)InventorySlot::RightHand);
  DrawInventorySlot(16*scale, screenHeight-(32+16)*scale, (size_t)InventorySlot::LeftHand);
  
  DrawInventorySlot(16*scale + 0*32*scale, screenHeight-(16)*scale, (size_t)InventorySlot::QuickSlot1);
  DrawInventorySlot(16*scale + 1*32*scale, screenHeight-(16)*scale, (size_t)InventorySlot::QuickSlot2);
  DrawInventorySlot(16*scale + 2*32*scale, screenHeight-(16)*scale, (size_t)InventorySlot::QuickSlot3);
  DrawInventorySlot(16*scale + 3*32*scale, screenHeight-(16)*scale, (size_t)InventorySlot::QuickSlot4);
  
  DrawInventorySlot(screenWidth - 16*scale - 3*32*scale, screenHeight-(16)*scale, (size_t)InventorySlot::QuickSlot5);
  DrawInventorySlot(screenWidth - 16*scale - 2*32*scale, screenHeight-(16)*scale, (size_t)InventorySlot::QuickSlot6);
  DrawInventorySlot(screenWidth - 16*scale - 1*32*scale, screenHeight-(16)*scale, (size_t)InventorySlot::QuickSlot7);
  DrawInventorySlot(screenWidth - 16*scale - 0*32*scale, screenHeight-(16)*scale, (size_t)InventorySlot::QuickSlot8);
  
  std::stringstream str;
  str << (GetAngles().EulerToVector()) << smoothPosition;
  
  RenderString rs(str.str());
  rs.Draw(0,0);
}

void
Player::MouseClick(int button, bool down) {
  if (button == GLFW_MOUSE_BUTTON_LEFT) this->itemActiveLeft = down;
  if (button == GLFW_MOUSE_BUTTON_RIGHT) this->itemActiveRight = down;
}

void 
Player::DrawInventorySlot(float x, float y, size_t slot) {
  float scale = 1;
  if (screenWidth <= 640) scale = 1;
  else scale = 2;

  if (slot == (size_t)InventorySlot::LeftHand)
    drawIcon(x, y, 16*scale, 16*scale, slotLeftHandTex);
  else if (slot == (size_t)InventorySlot::RightHand)
    drawIcon(x, y, 16*scale, 16*scale, slotRightHandTex);
  else
    drawIcon(x, y, 16*scale, 16*scale, slotTex);

  if (slot < inventory.size() && inventory[slot] != nullptr)
    inventory[slot]->DrawIcon(x, y, 16*scale, 16*scale);
}

const IColor &Player::GetTorchLight() {
  return torchLight;
}
