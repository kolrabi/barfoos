#ifndef BARFOOS_GAME_H
#define BARFOOS_GAME_H 

#include "common.h"
#include "feature.h"

class World;
class Player;
class Mob;

class Game final {
public:

  Game(const std::string &seed);
  ~Game();

  void Render() const;
  void Update(float t);

  float GetDeltaT() const { return deltaT; }
  
  void MouseClick(int button, bool down);

private:

  std::shared_ptr<World> world;
  std::shared_ptr<Player> player;

  float lastT;
  float deltaT;
  
  std::string seed;
  
  void BuildWorld(int level);
};


#endif

