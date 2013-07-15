#ifndef BARFOOS_GAME_H
#define BARFOOS_GAME_H 

#include "common.h"
#include "feature.h"
#include "random.h"
#include "2d.h"

class Game final {
public:

  Game(const std::string &seed, size_t level = 0, const Point &screenSize = Point(320, 240));
  Game(const Game &game) = delete;
  Game(Game &&game) = delete;
  ~Game();
  
  bool Init();
  bool Frame();
  
  Gfx    &GetGfx()    const { return *this->gfx;    }
  Input  &GetInput()  const { return *this->input;  }
  World  &GetWorld()  const { return *this->world;  }
  Random &GetRandom()       { return random;        }

  float   GetTime()   const { return this->lastT;   }
  float   GetDeltaT() const { return this->deltaT;  }
  float   GetFPS()    const { return this->fps;     }

  int     GetLevel()  const { return level;         }
  
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
  
  void Explosion(const IVector3 &pos, const IVector3 &size, float strength);
  void HandleEvent(const InputEvent &event);
  
private:

  Input *input;
  Gfx *gfx;
  World *world;
  bool isInit;
  
  size_t handlerId;
  
  void Deinit();
  
  void Render() const;
  void Update(float t, float deltaT);
  
  int level;
  
  std::map<size_t, Entity*> entities;
  Player *player;
  size_t nextEntityId;
  size_t GetNextEntityId() { return nextEntityId++; }
  
  std::shared_ptr<InventoryGui> inventoryGui;
  std::shared_ptr<Gui> activeGui;

  float startT;
  float lastT;
  float deltaT;
  size_t frame;
  float lastFPST;
  
  float fps;
  
  std::string seed;
  Random random;
  
  bool showInventory;
  
  void BuildWorld();
  std::vector<const Entity*> FindLightEntities(const Vector3 &pos, float radius) const;
};

#endif

