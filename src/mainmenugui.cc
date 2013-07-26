#include "mainmenugui.h"
#include "mainmenustate.h"
#include "gfx.h"

MainMenuGui::MainMenuGui(MainMenuState &state) :
  state(state)
{
  Gfx &gfx = state.GetGame().GetGfx();
  Point vscreen = gfx.GetVirtualScreenSize();
  
  Gui *btnExit = new Gui();
  btnExit->SetSize(Point(128, 32));
  btnExit->SetCenter(vscreen/2 + Point(0, 64));
  btnExit->SetGravity(false, false, false, false);
  btnExit->SetText("Exit");
  btnExit->SetOnActivate([&](Gui*){state.ExitGame();});
  btnExit->SetBackground(NinePatch("gui/button", Rect(Point(8,8), Point(16,16))));
  this->AddChild(btnExit);

  Gui *btnNewGame = new Gui();
  btnNewGame->SetSize(Point(128, 32));
  btnNewGame->SetCenter(vscreen/2 + Point(0, -64));
  btnNewGame->SetGravity(false, false, false, false);
  btnNewGame->SetText("New Game");
  btnNewGame->SetOnActivate([&](Gui*){state.NewGame();});
  btnNewGame->SetBackground(NinePatch("gui/button", Rect(Point(8,8), Point(16,16))));
  this->AddChild(btnNewGame);

  Gui *btnContinue = new Gui();
  btnContinue->SetSize(Point(128, 32));
  btnContinue->SetCenter(vscreen/2 + Point(0, 0));
  btnContinue->SetGravity(false, false, false, false);
  btnContinue->SetText("Continue");
  btnContinue->SetOnActivate([&](Gui*){state.ContinueGame();});
  btnContinue->SetBackground(NinePatch("gui/button", Rect(Point(8,8), Point(16,16))));
  this->AddChild(btnContinue);
}

MainMenuGui::~MainMenuGui() {
}


