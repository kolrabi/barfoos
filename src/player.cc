#include "player.h"
#include "world.h"
#include "cell.h"
#include "text.h"
#include "util.h"
#include "worldedit.h"
#include "weapon.h"

#include <GL/glfw.h>
#include <cmath>

static float eyeHeight = 0.7f;

extern float mouseDX, mouseDY;
extern int screenWidth, screenHeight;

Player::Player() {
  aabb.center = Vector3(16,16,16);
  aabb.extents = Vector3(0.2,0.8,0.2);
  velocity = Vector3(0,0,1);

  mass = 100;

  bobPhase = 0;
  bobAmplitude = 0;
  
  texture = 0;
  moveInterval = 0;
  
  maxSpeed = 5;

  noclip = false;
  
  this->activeItem = std::make_shared<Weapon>(Weapon());
  this->itemActiveLeft = false;
  this->itemActiveRight = false;
  this->crosshairTex = loadTexture("gui/crosshair");
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
  if (itemActiveLeft) {
    this->activeItem->Use(*this, pos, fwd, true);
  }
  if (itemActiveRight) {
    this->activeItem->Use(*this, pos, fwd, false);
  }
}

void Player::Draw() {
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
  maxSpeed = sneak?2:5;

  move = Vector3();
  
  if (glfwGetKey('D')) move = move + right * maxSpeed;
  if (glfwGetKey('A')) move = move - right * maxSpeed;
  if (glfwGetKey('W')) move = move + fwd * maxSpeed;
  if (glfwGetKey('S')) move = move - fwd * maxSpeed;

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

  glColor3ub(light.r, light.g, light.b);

  this->activeItem->Draw();
}

void 
Player::DrawGUI() {
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  glColor3ub(255, 255, 255);
  drawBillboard(Vector3(), 32.0/screenWidth, 32.0/screenHeight, crosshairTex);  
  
  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
}

void
Player::MouseClick(int button, bool down) {
  if (button == GLFW_MOUSE_BUTTON_LEFT) this->itemActiveLeft = down;
  if (button == GLFW_MOUSE_BUTTON_RIGHT) this->itemActiveRight = down;
}
