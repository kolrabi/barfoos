#include "gfx.h"
#include "game.h"
#include "item.h"
#include "input.h"
#include "effect.h"
#include "cell.h"
#include "feature.h"
#include "entity.h"
#include "gui.h"

#include "serializer.h"
#include "deserializer.h"

#include "mainmenustate.h"

#include <algorithm>

Game::Game(const Point &screenSize) : 
  isInit        (false),
  input         (new Input()), 
  gfx           (new Gfx(Point(1920, 32), screenSize, false)),
  handlerId     (this->input->AddHandler( [this](const InputEvent &event){ this->HandleEvent(event); } )),
  activeGameState(nullptr),
  nextGameState(nullptr),
  activeGui     (nullptr),
  startT        (0.0),
  lastT         (0.0),
  deltaT        (0.0),
  frame         (0),
  lastFPST      (0.0),
  fps           (0.0),
  seed          (""), 
  random        ("")
{
}

Game::~Game() {
  if (activeGameState) activeGameState->Leave(nullptr);
  delete activeGameState;
  
  this->input->RemoveHandler(this->handlerId);
  
  delete gfx;
  delete input;
}

bool 
Game::Init() {
  if (this->isInit) return true;
  
  Log("Initializing game\n");
  if (!this->gfx->Init(*this)) return false;
  
  this->lastT = 0;
  this->deltaT = 0;
  this->frame = 0;
  this->lastFPST = 0;
  this->fps = 0;

  this->isInit = true;
  return true;
}

void
Game::NewGame(const std::string &seed) {
  this->seed = seed;
  this->random.Seed(seed, 0);

  std::string scrolls = loadAssetAsString("text/scrolls");
  scrollMarkov.add(0, scrolls.begin(), scrolls.end());
  
  LoadCells();
  LoadFeatures();
  LoadEntities();
  LoadEffects();
  LoadItems(*this);
  
  MainMenuState *state = new MainMenuState(*this);
  nextGameState = state;
  
  this->startT = this->gfx->GetTime();
}

bool Game::Frame() {
  PROFILE();
  
  if (!this->gfx->Swap()) return false;
  
  this->gfx->ClearColor(IColor());
  this->gfx->ClearDepth(1.0);

  if (nextGameState != activeGameState) {
    if (activeGameState) activeGameState->Leave(nextGameState);
    activeGameState = nextGameState;
    if (activeGameState) activeGameState->Enter();
  }
  
  // update game (at most 0.1s at a time)
  float t = this->gfx->GetTime() - this->startT;
  while(t - this->lastT > 0.1) {
    this->lastT += 0.1;
    this->Update(lastT, 0.1);
    this->input->Update();
  }
  this->Update(t, t - this->lastT);
  this->lastT = t;
  
  // render game
  this->Render();

  this->gfx->Update(*this);
  
  this->frame ++;
  if (t - this->lastFPST >= 1.0) {
    this->fps = this->frame / (t-this->lastFPST);
    this->frame = 0;
    this->lastFPST += 1.0;
  }
  return nextGameState != nullptr;
}

void
Game::Render() const {
  PROFILE();

  if (activeGameState) activeGameState->Render(*this->gfx);  

  // next draw gui stuff
  this->gfx->GetView().GUI();
  
  if (this->activeGui) {
    this->activeGui->Draw(*this->gfx, Point());
    this->activeGui->DrawTooltip(*this->gfx, Point());
  }
}

void 
Game::Update(float t, float deltaT) {
  PROFILE();

  this->lastT = t;
  this->deltaT = deltaT;
  
  if (activeGameState) {
    nextGameState = activeGameState->Update();
  }
 
  if (this->activeGui) this->activeGui->Update(*this);
  
  this->input->Update();
}

void Game::HandleEvent(const InputEvent &event) {
  if (this->activeGui) {
    this->activeGui->HandleEvent(event);
  } else if (this->activeGameState) {
    this->activeGameState->HandleEvent(event);
  }
}

std::string 
Game::GetScrollName() {
  if (scrollMarkov.size() == 0) return "ERROR";

  std::string name = "Scroll of ";
  char c = 0;
  size_t l = 0;
  while(true) {
    c = scrollMarkov[c].select(this->random.Float01());
    if (c == '\n') c = ' ';
    if (c == ' ' && l > 10) break;
    name += c;
    l++;
  }
  return name;
}

void Game::SetIdentified(const std::string &name) {
  if (this->IsIdentified(name)) return;
  this->identifiedItems.push_back(name);
}

bool Game::IsIdentified(const std::string &name) const {
  return std::find(this->identifiedItems.begin(), this->identifiedItems.end(), name) != this->identifiedItems.end();
}

void
Game::SetGui(const std::shared_ptr<Gui> &gui) {
  if (this->activeGui) {
    this->activeGui->OnHide();
    this->gfx->DecGuiCount();
  }
  this->activeGui = gui;
  if (this->activeGui) {
    this->activeGui->OnShow();
    this->gfx->IncGuiCount();
  }
}

Serializer &operator << (Serializer &ser, const Game &game) {
  ser << game.lastT;
  ser << game.seed;
  ser << game.identifiedItems;
  return ser;
}

Deserializer &operator >> (Deserializer &deser, Game &game) {
  deser >> game.lastT;
  deser >> game.seed;
  deser >> game.identifiedItems;

  game.startT = game.GetGfx().GetTime()-game.lastT;
  game.random.Seed(game.seed, 0);

  std::string scrolls = loadAssetAsString("text/scrolls");
  game.scrollMarkov.clear();
  game.scrollMarkov.add(0, scrolls.begin(), scrolls.end());
  
  LoadCells();
  LoadFeatures();
  LoadEntities();
  LoadEffects();
  LoadItems(game);
  
  return deser;
}
