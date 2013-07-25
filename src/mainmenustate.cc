#include "mainmenustate.h"
#include "runningstate.h"
#include "mainmenugui.h"
#include "text.h"
#include "gfx.h"

MainMenuState::MainMenuState(Game &game) :
  GameState(game),
  mainMenuGui(new MainMenuGui(*this)),
  nextState(this)
{
}
  
MainMenuState::~MainMenuState() {
}
  
void MainMenuState::Enter() {
  GetGame().SetGui(this->mainMenuGui);
}

void MainMenuState::Leave(GameState *) {
  GetGame().SetGui(nullptr);
}

GameState *MainMenuState::Update() {
  return nextState;
}

void MainMenuState::Render(Gfx &gfx) const {
  gfx.ClearColor(IColor(30, 30, 20));
  gfx.ClearDepth(1.0);
  gfx.Viewport(Rect());
  gfx.GetView().GUI();
  gfx.SetShader("gui");
}

void MainMenuState::HandleEvent(const InputEvent &) {
}

void MainMenuState::ExitGame() {
  nextState = nullptr;
}

void MainMenuState::NewGame() {
  RunningState *runningState = new RunningState(GetGame());
  runningState->NewGame();
  
  nextState = runningState;
}

void MainMenuState::ContinueGame() {
  RunningState *runningState = new RunningState(GetGame());
  runningState->ContinueGame();
  
  nextState = runningState;
}
