#ifndef BARFOOS_GAMESTATE_H
#define BARFOOS_GAMESTATE_H

#include "game.h"

class GameState {
public:
  GameState(Game &game) : game(game) {};
  virtual ~GameState() {};

  virtual void          Enter ()            = 0;
  virtual void          Leave (GameState *) = 0;
  virtual GameState *   Update()            = 0;
  virtual void          Render(Gfx &)       const = 0;

  Game &                GetGame()                       { return game; }
  Random &              GetRandom()                     { return game.GetRandom(); }
  virtual void          HandleEvent(const InputEvent &) {};

private:

  Game &game;
};

#endif
