#ifndef BARFOOS_GAME_H
#define BARFOOS_GAME_H

#include "random.h"
#include "2d.h"

#include "markov.h"

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

  float   GetTime()   const { return this->lastT;   }
  float   GetDeltaT() const { return this->deltaT;  }
  float   GetFPS()    const { return this->fps;     }

  std::string GetScrollName();
  void SetIdentified(const std::string &name);
  bool IsIdentified(const std::string &name) const;

  void HandleEvent(const InputEvent &event);
  void                  SetGui(const std::shared_ptr<Gui> &gui);

private:

  bool    isInit;

  Input   *input;
  Gfx     *gfx;
  Audio   *audio;

  size_t  handlerId;

  GameState *activeGameState;
  GameState *nextGameState;

  // --------------------------------------------

  std::shared_ptr<Gui> activeGui;

  float   startT;

  float   lastT;
  float   deltaT;

  size_t  frame, realFrame;
  float   lastFPST;

  float   fps;

  std::string seed;
  Random random;

  void Render() const;
  void Update(float t, float deltaT);

  markov_chain<char> scrollMarkov;
  std::vector<std::string> identifiedItems;

  friend Serializer &operator << (Serializer &ser, const Game &game);
  friend Deserializer &operator >> (Deserializer &deser, Game &game);
};

#endif

