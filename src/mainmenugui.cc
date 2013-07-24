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
  this->AddChild(btnExit);

  Gui *btnNewGame = new Gui();
  btnNewGame->SetSize(Point(128, 32));
  btnNewGame->SetCenter(vscreen/2 + Point(0, -64));
  btnNewGame->SetGravity(false, false, false, false);
  btnNewGame->SetText("New Game");
  btnNewGame->SetOnActivate([&](Gui*){state.NewGame();});
  this->AddChild(btnNewGame);
}

MainMenuGui::~MainMenuGui() {
}


