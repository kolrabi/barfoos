#ifndef BARFOOS_MAINMENU_GUI_H
#define BARFOOS_MAINMENU_GUI_H

#include "gui/gui.h"

class MainMenuState;

class MainMenuGui : public Gui {
public: 
  MainMenuGui(MainMenuState &);
  virtual ~MainMenuGui();

  MainMenuGui &operator=(const MainMenuGui &) = delete;
  
private:

  MainMenuState &state;
};

#endif

