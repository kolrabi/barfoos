#include <GL/glfw.h>

#include "game.h"
#include "world.h"
#include "player.h"
#include "worldedit.h"
#include "feature.h"
#include "random.h"
#include "inventorygui.h"

extern int screenWidth;
extern int screenHeight;

Game::Game(const std::string &seed, size_t level) : seed(seed), random(seed, level) {
  LoadCells();
  LoadFeatures();
  LoadEntities();
  LoadItems();
 
  this->BuildWorld(level);
  
  this->player = std::make_shared<Player>(Player());
  this->player->SetPosition(IVector3(32,32,32));
  this->player->SetSpawnPos(IVector3(32,32,32));
  this->world->AddPlayer(this->player);

  this->inventoryGui = std::shared_ptr<InventoryGui>(new InventoryGui(this->player));
  this->showInventory = false;
  this->lastT = 0;
      this->activeGui = this->inventoryGui;
}

Game::~Game() {
}

void
Game::Render() const {
  glClearColor(0.3,0.3,0.2, 1.0);

  // draw world first
  glViewport(0,0, screenWidth, screenHeight);
  glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(60.0f, (float)screenWidth/(float)screenHeight, 0.015f, 64.0f);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  if (glfwGetKey('Q')) {
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  } else {
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  }
  glFogi(GL_FOG_MODE, GL_EXP2);
  glFogf(GL_FOG_DENSITY, 0.051f);
  player->View();
  world->Draw();
  glClear(GL_DEPTH_BUFFER_BIT);
  player->DrawWeapons();

  // next draw mini map
  if (glfwGetKey('M')) {
    glScissor(screenWidth/2-screenHeight/2,0,screenHeight,screenHeight);
    glViewport(screenWidth/2-screenHeight/2,0,screenHeight,screenHeight);
  } else {
    glViewport(screenWidth-screenWidth/8,screenHeight-screenWidth/8, screenWidth/8, screenWidth/8);
    glScissor(screenWidth-screenWidth/8,screenHeight-screenWidth/8, screenWidth/8, screenWidth/8);
  }
  glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);

//  glDisable(GL_FOG);
  glFogi(GL_FOG_MODE, GL_LINEAR);
  player->MapView();
  world->DrawMap();
//  glEnable(GL_FOG);

  glScissor(0,0,screenWidth,screenHeight);
  glViewport(0,0,screenWidth,screenHeight);

  // next draw debug stuff
  glDisable(GL_DEPTH_TEST);
  glColor3ub(255,255,0);
  player->DrawGUI();
  
  if (this->activeGui) {
    viewGUI();
    this->activeGui->Draw(Point());
  }
}

void 
Game::Update(float t) {
  if (lastT == 0) lastT = t;
  deltaT = t - lastT;
  world->Update(t);
  lastT = t;

  if (glfwGetKey(GLFW_KEY_TAB)) {
    if (!this->showInventory) {
      guiActive ++;
      if (this->activeGui) this->activeGui->OnHide();
      this->activeGui = this->inventoryGui;
      this->activeGui->OnShow();
    }
    this->showInventory = true;
  } else {
    if (this->showInventory) {
      guiActive --;
      if (this->activeGui) this->activeGui->OnHide();
      this->activeGui = nullptr;
    }
    this->showInventory = false;
  }

  if (this->activeGui) this->activeGui->Update(t);
}

void 
Game::BuildWorld(size_t level) { 
  random.Seed(seed, level); 
  this->world = std::shared_ptr<World>(new World(IVector3(64, 64, 64), level, random));

}

void Game::OnMouseMove(const Point &pos) {
  if (this->activeGui) {
    this->activeGui->OnMouseMove(pos);
  }
}

void Game::OnMouseClick(const Point &pos, int button, bool down) {
  if (this->activeGui) {
    this->activeGui->OnMouseClick(pos, button, down);
  } else {
    this->player->MouseClick(pos, button, down);
  }
}

