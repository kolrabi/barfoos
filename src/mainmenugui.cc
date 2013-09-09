#include "common.h"

#include "mainmenugui.h"

#include "mainmenustate.h"
#include "gfx.h"
#include "audio.h"

MainMenuGui::MainMenuGui(MainMenuState &state) :
  state(state)
{
  Gfx &gfx = state.GetGame().GetGfx();
  Point vscreen = gfx.GetScreen().GetVirtualSize();

  Rect btnRect(Point(8,8), Point(16,16));
  NinePatch npBtnEnabled ("gui/button.enabled",  btnRect);
  NinePatch npBtnDisabled("gui/button.disabled", btnRect);
  NinePatch npBtnHover   ("gui/button.hover",    btnRect);
  NinePatch npBtnActive  ("gui/button.active",   btnRect);

  Gui *btnExit = new Gui();
  btnExit->SetSize(Point(128, 32));
  btnExit->SetCenter(vscreen/2 + Point(0, 64));
  btnExit->SetGravity(false, false, false, false);
  btnExit->SetText("Exit");
  btnExit->SetOnActivate([&](Gui*){state.ExitGame();});
  btnExit->SetBackground(GuiState::Enabled,   npBtnEnabled);
  btnExit->SetBackground(GuiState::Disabled, npBtnDisabled);
  btnExit->SetBackground(GuiState::Hover,    npBtnHover);
  btnExit->SetBackground(GuiState::Active,   npBtnActive);
  this->AddChild(btnExit);

  Gui *btnNewGame = new Gui();
  btnNewGame->SetSize(Point(128, 32));
  btnNewGame->SetCenter(vscreen/2 + Point(0, -64));
  btnNewGame->SetGravity(false, false, false, false);
  btnNewGame->SetText("New Game");
  btnNewGame->SetOnActivate([&](Gui*){
    state.NewGame();
    });
  btnNewGame->SetBackground(GuiState::Enabled,   npBtnEnabled);
  btnNewGame->SetBackground(GuiState::Disabled, npBtnDisabled);
  btnNewGame->SetBackground(GuiState::Hover,    npBtnHover);
  btnNewGame->SetBackground(GuiState::Active,   npBtnActive);
  this->AddChild(btnNewGame);

  Gui *btnContinue = new Gui();
  btnContinue->SetSize(Point(128, 32));
  btnContinue->SetCenter(vscreen/2 + Point(0, 0));
  btnContinue->SetGravity(false, false, false, false);
  btnContinue->SetText("Continue");
  btnContinue->SetOnActivate([&](Gui*){state.ContinueGame();});
  btnContinue->SetBackground(GuiState::Enabled,   npBtnEnabled);
  btnContinue->SetBackground(GuiState::Disabled, npBtnDisabled);
  btnContinue->SetBackground(GuiState::Hover,    npBtnHover);
  btnContinue->SetBackground(GuiState::Active,   npBtnActive);
  //btnContinue->SetEnabled(false);
  this->AddChild(btnContinue);

  Gui *lblTitle = new Gui();
  lblTitle->SetSize(Point(vscreen.x, 32));
  lblTitle->SetCenter(vscreen/2 - Point(0, vscreen.y/3));
  lblTitle->SetGravity(true, true, true, true);
  lblTitle->SetText("Untitled Game", "big");
  lblTitle->SetColor(IColor(255,255,255));
  this->AddChild(lblTitle);
}

MainMenuGui::~MainMenuGui() {
}


