#include "common.h"

#include "game.h"

#include "entity.h"
#include "item.h"
#include "effect.h"
#include "spell.h"

#include "cell.h"
#include "feature.h"

#include "gfx.h"
#include "gfxview.h"

#include "audio.h"

#include "input.h"

#include "gui.h"

#include "mainmenustate.h"
#include "matrix4.h"

#include "fileio.h"

Game::Game(const Point &screenSize) :
  isInit        (false),
  input         (new Input()),
#if WIN32
  gfx           (new Gfx(Point(1920, 32), screenSize, false)),
#else
  gfx           (new Gfx(Point(20, 32), screenSize, false)),
#endif
  audio         (new Audio()),
  handlerId     (this->input->AddHandler( [this](const InputEvent &event){ this->HandleEvent(event); } )),
  activeGameState(nullptr),
  nextGameState(nullptr),
  activeGui     (nullptr),
  startT        (0.0),
  deltaT        (0.0),
  frame         (0),
  realFrame     (0),
  lastFPST      (0.0),
  fps           (0.0),
  random        ("")
{
}

Game::~Game() {
  if (activeGameState) activeGameState->Leave(nullptr);
  delete activeGameState;

  this->input->RemoveHandler(this->handlerId);

  delete gfx;
  delete audio;
  delete input;
}

bool
Game::Init() {
  if (this->isInit) return true;

  Log("Initializing game\n");
  if (!this->gfx->Init(*this)) return false;
  if (!this->audio->Init()) return false;

  this->proto.set_last_time(0.0f);
  this->proto.set_seed("");
  this->proto.clear_identified_items();

  this->deltaT    = 0;
  this->frame     = 0;
  this->realFrame = 0;
  this->lastFPST  = 0;
  this->fps       = 0;

  this->isInit    = true;

  MainMenuState *state = new MainMenuState(*this);
  nextGameState   = state;

  return true;
}

void
Game::NewGame(const std::string &seed) {
  this->proto.set_seed(seed);
  this->random.Seed(seed, 0);

  std::string scrolls = loadAssetAsString("text/scrolls");
  scrollMarkov.add(0, scrolls.begin(), scrolls.end());

  LoadCells();
  LoadFeatures();
  LoadEntities();
  LoadEffects();
  LoadItems(*this);
  LoadSpells();

  this->startT = this->gfx->GetTime();
}

bool 
Game::Frame() {
  PROFILE();

  if (nextGameState != activeGameState) {
    if (activeGameState) {
      Log("leaving %s gamestate\n", typeid(*activeGameState).name());
      activeGameState->Leave(nextGameState);
    }
    activeGameState = nextGameState;
    if (activeGameState) {
      Log("entering %s gamestate\n", typeid(*activeGameState).name());
      activeGameState->Enter();
    }
    Log("changed gamestate\n");
  }

//  float t = lastT + 1.0/25.0;

  // update game (at most 0.1s at a time)
  float t = this->gfx->GetTime() - this->startT;

  // if too laggy, skip ahead
  if (t - this->proto.last_time() > 0.5) {
    float skip = t - this->proto.last_time() - 0.1;
    Log("update is too slow, skipping %f seconds\n", skip);
    this->startT += skip;
    t = this->gfx->GetTime() - this->startT;
  }

  // while only a little laggy, catch up
  while(t - this->proto.last_time() > 0.1) {
    Log("update is slow, %fs (%f)\n", t - this->proto.last_time(), t);
    this->proto.set_last_time(this->proto.last_time() + 0.1f);
    this->Update(this->proto.last_time(), 0.1);
    this->input->Update();
  }

  this->Update(t, t - this->proto.last_time());
  this->proto.set_last_time(t);

  // render game
  this->Render();

  /*
  char tmp[32];
  snprintf(tmp, sizeof(tmp), "frame%04u.png", (unsigned int)this->realFrame);
  this->gfx->SaveScreen(tmp);
  */

  this->gfx->Update(*this);
  this->audio->Update(*this);

  this->frame ++;
  this->realFrame++;
  if (t - this->lastFPST >= 1.0) {
    this->fps = this->frame / (t-this->lastFPST);
    this->frame = 0;
    this->lastFPST += 1.0;
  }

  if (!this->gfx->GetScreen().Swap()) nextGameState = nullptr;

  this->gfx->ClearColor(IColor());
  this->gfx->ClearDepth(1.0);

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
Game::Update(float, float deltaT) {
  PROFILE();

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

  std::string name = "scroll of ";
  char c = ' ';
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

void 
Game::SetIdentified(const std::string &name) {
  if (this->IsIdentified(name)) return;
  this->proto.add_identified_items(name);
}

bool 
Game::IsIdentified(const std::string &name) const {
  for (auto &i:this->proto.identified_items()) {
    if (i == name) return true;
  }
  return false;
}

void
Game::SetGui(const std::shared_ptr<Gui> &gui) {
  if (this->activeGui) {
    this->activeGui->OnHide();
    this->gfx->GetScreen().DecGuiCount();
  }
  this->activeGui = gui;
  if (this->activeGui) {
    this->activeGui->OnShow();
    this->gfx->GetScreen().IncGuiCount();
  }
}

void
Game::Serialize(std::ostream &out) {
  this->proto.SerializeToOstream(&out);
}

void
Game::Deserialize(std::istream &in) {
  this->proto.ParseFromIstream(&in);

  this->startT = this->GetGfx().GetTime()-this->GetTime();
  this->random.Seed(this->proto.seed(), 0);

  std::string scrolls = loadAssetAsString("text/scrolls");
  this->scrollMarkov.clear();
  this->scrollMarkov.add(0, scrolls.begin(), scrolls.end());

  LoadCells();
  LoadFeatures();
  LoadEntities();
  LoadEffects();
  LoadItems(*this);
}
