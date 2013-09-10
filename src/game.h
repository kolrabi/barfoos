#ifndef BARFOOS_GAME_H
#define BARFOOS_GAME_H

#include "random.h"
#include "2d.h"

#include "markov.h"

#include "game.pb.h"

#include <iostream>

class GameState;

class Game final {
public:

  Game(const Point &screenSize = Point(640, 480));
  Game(const Game &game) = delete;
  Game(Game &&game) = delete;
  ~Game();

  Game &operator=(Game &game) = delete;

  bool Init();
  void NewGame(const std::string &seed);
  bool Frame();

  Gfx    &GetGfx()    const { return *this->gfx;    }
  Audio  &GetAudio()  const { return *this->audio;  }
  Input  &GetInput()  const { return *this->input;  }
  Random &GetRandom()       { return random;        }

  float   GetTime()   const { return this->proto.last_time();   }
  float   GetDeltaT() const { return this->deltaT;  }
  float   GetFPS()    const { return this->fps;     }

  std::string GetScrollName();
  void SetIdentified(const std::string &name);
  bool IsIdentified(const std::string &name) const;

  void HandleEvent(const InputEvent &event);
  void                  SetGui(const std::shared_ptr<Gui> &gui);

  void Serialize(std::ostream &out);
  void Deserialize(std::istream &in);

private:

  bool    isInit;

  Input   *input;
  Gfx     *gfx;
  Audio   *audio;

  size_t  handlerId;

  GameState *activeGameState;
  GameState *nextGameState;

  Game_Proto proto;

  // --------------------------------------------

  std::shared_ptr<Gui> activeGui;

  float   startT;
  float   deltaT;

  size_t  frame, realFrame;
  float   lastFPST;

  float   fps;

  Random random;

  void Render() const;
  void Update(float t, float deltaT);

  markov_chain<char> scrollMarkov;

  //friend Serializer &operator << (Serializer &ser, const Game &game);
 //friend Deserializer &operator >> (Deserializer &deser, Game &game);
};

#endif

