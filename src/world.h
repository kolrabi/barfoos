#ifndef BARFOOS_WORLD_H
#define BARFOOS_WORLD_H

#include "common.h"
#include "cell.h"
#include "util.h"
#include "feature.h"

class Entity;
class Player;
class Random;
class Shader;

class World final {
public:

  World(const IVector3 &size, int level, Random &random);
  ~World();
  
  const IVector3 &GetSize() const { return size; }

  void Draw();
  void DrawMap();
  void Update(float t);
  
  void AddEntity(const std::shared_ptr<Entity> &entity);
  void AddPlayer(const std::shared_ptr<Player> &entity);
  void RemoveEntity(const std::shared_ptr<Entity> &entity);
  bool CheckMob(const IVector3 &pos);
  std::vector<std::shared_ptr<Entity>> FindEntities(const AABB &aabb);

  Cell &GetCell(const IVector3 &pos) const;
  Cell &SetCell(const IVector3 &pos, const Cell &cell, bool ignoreLock = false);
  std::vector<const Cell *> GetCellNeighbours(const IVector3 &pos) const;
  IColor GetLight(const IVector3 &pos) const;

  bool CastRayX(const Vector3 &org, float dir);
  bool CastRayZ(const Vector3 &org, float dir);
  float CastRayYUp(const Vector3 &org);
  float CastRayYDown(const Vector3 &org);

  Cell &CastRayCell(const Vector3 &org, const Vector3 &dir, float &distance, Side &side);
  bool IsPointSolid(const Vector3 &org);
  bool IsAABBSolid(const AABB &aabb);

  Vector3 MoveAABB(const AABB &aabb, const Vector3 &dir, uint8_t &axis);
  
  void BeginCheckOverwrite() { checkOverwrite = true; checkOverwriteOK = true; }
  bool FinishCheckOverwrite() { checkOverwrite = false; return checkOverwriteOK;}
  bool IsChecking() const { return checkOverwrite; }

  void Dump();

  int GetLevel() const { return level; }
  bool IsDefault(const IVector3 &pos);

  float GetDeltaT() const { return deltaT; }

  void AddFeatureSeen(size_t f);

private:

  void UpdateCell(size_t i);
  void UpdateCell(const IVector3 &pos);
  
  size_t GetCellIndex(const IVector3 &pos) const { return pos.x+size.x*(pos.y+size.y*pos.z); }
  IVector3 GetCellPos(size_t i) const { return IVector3( i%size.x, (i/size.x)%size.y, (i/(size.x*size.y))%size.z); }
  bool IsValidCellPosition(const IVector3 &pos) const { 
    return pos.x < size.x  && pos.y < size.y && pos.z < size.z; 
  }

  Random &random;
  IVector3 size;
  bool dirty, firstDirty;

  size_t cellCount;
  std::vector<Cell> cells;
  Cell defaultCell;
  std::vector<bool> defaultMask;

  std::vector<size_t> dynamicCells;
  float lastT;
  float deltaT;
  float nextTickT;
  float tickInterval;

  IColor ambientLight;
  
  std::vector<std::shared_ptr<Entity>> entities;
  std::shared_ptr<Player> player;

  std::map<unsigned int, std::vector<Vertex>> vertices;
  std::vector<unsigned int> vbos;  
 
  std::vector<bool> seenFeatures;

  bool checkOverwrite;
  bool checkOverwriteOK;

  int level;
  std::vector<FeatureInstance> instances;

  Shader *defaultShader;
};

inline Cell &
World::GetCell(const IVector3 &pos) const {
  if (checkOverwrite) return const_cast<Cell &>(this->defaultCell);
  if (!this->IsValidCellPosition(pos)) return const_cast<Cell &>(this->defaultCell);
  return const_cast<Cell &>(this->cells[this->GetCellIndex(pos)]);
}

#endif

