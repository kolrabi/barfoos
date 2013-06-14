#ifndef BARFOOS_GAME_H
#define BARFOOS_GAME_H 

#include "common.h"
#include "feature.h"
#include "random.h"

class World;
class Player;
class Mob;

class Game final {
public:

  Game(const std::string &seed, size_t level = 0);
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
  Random random;
  
  void BuildWorld(size_t level);
};


#endif

