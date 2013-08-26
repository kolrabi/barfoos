#ifndef BARFOOS_MAINMENUSTATE_H
#define BARFOOS_MAINMENUSTATE_H

#include "game.h"

class MainMenuGui;

class MainMenuState : public GameState {
public:

                        MainMenuState(Game &);
  virtual               ~MainMenuState();

  virtual void          Enter ()                    override;
  virtual void          Leave (GameState *next)     override;
  virtual GameState *   Update()                    override;
  virtual void          Render(Gfx &)               const override;
  virtual void          HandleEvent(const InputEvent &) override;

  void ExitGame();
  void NewGame();
  void ContinueGame();

private:

  std::shared_ptr<MainMenuGui> mainMenuGui;
  GameState *nextState;
};

#endif

