#ifndef BARFOOS_GAME_H
#define BARFOOS_GAME_H 

#include "common.h"
#include "feature.h"
#include "random.h"
#include "2d.h"

#include <unordered_map>
#include "markov.h"

class Game final {
public:

  Game(const Point &screenSize = Point(320, 240));
  Game(const Game &game) = delete;
  Game(Game &&game) = delete;
  ~Game();
  
  Game &operator=(Game &game) = delete;
  
  bool Init();
  void NewGame(const std::string &seed);
  bool Frame();
  
  Gfx    &GetGfx()    const { return *this->gfx;    }
  Input  &GetInput()  const { return *this->input;  }
  World  &GetWorld()  const { return *this->world;  }
  Random &GetRandom()       { return random;        }

  float   GetTime()   const { return this->lastT;   }
  float   GetDeltaT() const { return this->deltaT;  }
  float   GetFPS()    const { return this->fps;     }

  int     GetLevel()  const { return level;         }
  
  std::string GetScrollName();
  
  // entity management
  size_t  AddEntity(Entity *entity);
  Entity *GetEntity(size_t id);
  void    RemoveEntity(size_t entity);
  
  size_t  AddPlayer(Player *entity);
  Player &GetPlayer() { return *this->player; }
  
  bool    CheckEntities(const IVector3 &pos);
  
  std::vector<size_t> FindEntities(const AABB &aabb) const;
  std::vector<size_t> FindSolidEntities(const AABB &aabb) const;

  // misc.
  Vector3 MoveAABB(const AABB &aabb, const Vector3 &dir, uint8_t &axis);
  
  void Explosion(Entity &entity, const Vector3 &pos, size_t radius, float strength, float damage, Element element);
  
  void HandleEvent(const InputEvent &event);
  
private:

  bool    isInit;
  
  Input   *input;
  Gfx     *gfx;
  World   *world;
  
  size_t  handlerId;
  
  int     level;
  
  std::unordered_map<size_t, Entity*> entities;
  Player *player;
  size_t  nextEntityId;
  
  std::shared_ptr<InventoryGui> inventoryGui;
  std::shared_ptr<Gui> activeGui;

  float   startT;
  float   lastT;
  float   deltaT;
  size_t  frame;
  float   lastFPST;
  
  float   fps;
  
  std::string seed;
  Random random;
  
  bool showInventory;
  
  void Deinit();
  
  void Render() const;
  void Update(float t, float deltaT);
  
  size_t GetNextEntityId() { return nextEntityId++; }
  
  void BuildWorld();
  std::vector<const Entity*> FindLightEntities(const Vector3 &pos, float radius) const;
  
  markov_chain<char> scrollMarkov;
};

#endif

