#ifndef BARFOOS_WORLD_H
#define BARFOOS_WORLD_H

#include "common.h"
#include "cell.h"
#include "icolor.h"

#include <unordered_map>
#include <unordered_set>

class World final {
public:

  World(RunningState &state, const IVector3 &size);
  World(RunningState &state, Deserializer &deser);
  World(const World &world) = delete;
  World(World &&world) = delete;
  ~World();
  
  World &operator=(const World &) = delete;
  
  void Build();
  
  const IVector3 &GetSize() const { return size; }
  RunningState &GetState() const { return state; }

  void Draw(Gfx &gfx);
  void DrawMap(Gfx &gfx);
  void Update(RunningState &runningState);
  
  Cell &GetCell(const IVector3 &pos) const;
  Cell &SetCell(const IVector3 &pos, const Cell &cell, bool ignoreLock = false);
  std::vector<const Cell *> GetCellNeighbours(const IVector3 &pos) const;
  IColor GetLight(const IVector3 &pos) const;

  bool CastRayX(const Vector3 &org, float dir);
  bool CastRayZ(const Vector3 &org, float dir);
  float CastRayYUp(const Vector3 &org);
  float CastRayYDown(const Vector3 &org);

  Cell &CastRayCell(const Vector3 &org, const Vector3 &dir, float &distance, Side &side, size_t flags = CellFlags::Pickable);
  bool IsPointSolid(const Vector3 &org);
  bool IsAABBSolid(const AABB &aabb);

  Vector3 MoveAABB(const AABB &aabb, const Vector3 &target);
  Vector3 MoveAABB(const AABB &aabb, const Vector3 &target, uint8_t &axis, Cell **cell = nullptr, Side *side = nullptr);
  
  void BeginCheckOverwrite() { checkOverwrite = true; checkOverwriteOK = true; }
  bool FinishCheckOverwrite() { checkOverwrite = false; return checkOverwriteOK;}
  bool IsChecking() const { return checkOverwrite; }

  void Dump();

  bool IsDefault(const IVector3 &pos);
  
  void AddFeatureSeen(size_t f); // TODO: move to separate map class
  bool IsFeatureSeen(size_t id) const;
  void BreakBlock(const IVector3 &pos);
  
  void SetDirty() { this->dirty = true; }

  bool IsCellWalkable(const IVector3 &pos) const;
  bool IsCellValidTeleportTarget(const IVector3 &pos) const;
  IVector3 FindSolidBelow(const IVector3 &pos) const;
  IVector3 GetRandomTeleportTarget(Random &random) const;
  
  void MarkForUpdateNeighbours(const Cell &cell);

private:

  RunningState &state;
  IVector3 size;
  bool dirty, firstDirty;

  size_t cellCount;
  std::vector<Cell> cells;
  Cell defaultCell;
  std::vector<bool> defaultMask;

  std::vector<size_t> dynamicCells;
  float nextTickT;
  float tickInterval;

  std::unordered_set<size_t> neighbourUpdates;
  
  IColor ambientLight;
  
  std::vector<Vertex> allVerts;
  std::unordered_map<const Texture *, size_t> vertexStartsNormal;
  std::unordered_map<const Texture *, size_t> vertexCountsNormal;
  std::unordered_map<const Texture *, size_t> vertexStartsEmissive;
  std::unordered_map<const Texture *, size_t> vertexCountsEmissive;
  unsigned int vbo;  
 
  std::vector<bool> seenFeatures;

  bool checkOverwrite;
  bool checkOverwriteOK;

  void UpdateCell(size_t i);
  void UpdateCell(const IVector3 &pos);
  
  size_t GetCellIndex(const IVector3 &pos) const { return pos.x+size.x*(pos.y+size.y*pos.z); }
  IVector3 GetCellPos(size_t i) const { return IVector3( i%size.x, (i/size.x)%size.y, (i/(size.x*size.y))%size.z); }
  bool IsValidCellPosition(const IVector3 &pos) const { 
    return pos.x < size.x  && pos.y < size.y && pos.z < size.z; 
  }

  friend Serializer &operator << (Serializer &ser, const World &world);
};

inline Cell &
World::GetCell(const IVector3 &pos) const {
  if (checkOverwrite) return const_cast<Cell &>(this->defaultCell);
  if (!this->IsValidCellPosition(pos)) return const_cast<Cell &>(this->defaultCell);
  return const_cast<Cell &>(this->cells[this->GetCellIndex(pos)]);
}

#endif

