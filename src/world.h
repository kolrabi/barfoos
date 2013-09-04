#ifndef BARFOOS_WORLD_H
#define BARFOOS_WORLD_H

#include "common.h"
#include "cell.h"
#include "icolor.h"
#include "vertexbuffer.h"

#include <unordered_map>
#include <unordered_set>

class MiniMap final {
public:

  MiniMap(const World &world);
  MiniMap(const World &world, Deserializer &deser);

  void Draw(Gfx &gfx, const Vector3 &eyePos, float angle);

  void AddFeatureSeen(ID f);
  bool IsFeatureSeen(ID id) const;

private:

  const World &world;
  std::vector<bool> seenFeatures;
  const Texture *mapTexture;

  size_t viewY;
  void RepaintMap();

  friend Serializer &operator << (Serializer &ser, const MiniMap &map);
};

class World final {
public:

  World(RunningState &state, const IVector3 &size);
  World(RunningState &state, Deserializer &deser);
  World(const World &world) = delete;
  World(World &&world) = delete;
  ~World();

  World &operator=(const World &) = delete;

  const IVector3 &GetSize()   const { return size; }
  size_t    GetCellCount() const { return this->cellCount; }
  RunningState &  GetState()  const { return state; }
  MiniMap &       GetMap()          { return minimap; }

  void Draw(Gfx &gfx);
  void Update(RunningState &runningState);

  Cell &GetCell(const IVector3 &pos) const;
  Cell &SetCell(const IVector3 &pos, const Cell &cell, bool ignoreLock = false);
  std::vector<const Cell *> GetCellNeighbours(const IVector3 &pos) const;
  IColor GetLight(const IVector3 &pos) const;
  IColor GetLight(const Vector3 &pos) const;

  bool CastRayX(const Vector3 &org, float dir, bool sneak = false) const;
  bool CastRayZ(const Vector3 &org, float dir, bool sneak = false) const;
  float CastRayYUp(const Vector3 &org) const;
  float CastRayYDown(const Vector3 &org) const;

  Cell &CastRayCell(const Vector3 &org, const Vector3 &dir, float &distance, Side &side, size_t flags = CellFlags::Pickable) const;
  bool IsPointSolid(const Vector3 &org) const;
  bool IsAABBSolid(const AABB &aabb) const;

  Vector3 MoveAABB(const AABB &aabb, const Vector3 &target, bool sneak = false);
  Vector3 MoveAABB(const AABB &aabb, const Vector3 &target, uint8_t &axis, Cell **cell = nullptr, Side *side = nullptr, bool sneak = false);

  void BeginCheckOverwrite() { checkOverwrite = true; checkOverwriteOK = true; }
  bool FinishCheckOverwrite() { checkOverwrite = false; return checkOverwriteOK;}
  bool IsChecking() const { return checkOverwrite; }

  void Dump();

  bool IsDefault(const IVector3 &pos) const;
  void ClearDefaults();
  void CopyCellsFrom(const std::vector<Cell> &cells);

  void BreakBlock(const IVector3 &pos);

  void SetDirty() { this->dirty = true; }

  bool IsCellWalkable(const IVector3 &pos) const;
  bool IsCellValidTeleportTarget(const IVector3 &pos, const Vector3 &extents = Vector3(0,0,0)	) const;
  bool IsCellValidCeiling(const IVector3 &pos) const;
  IVector3 FindSolidBelow(const IVector3 &pos) const;
  IVector3 FindSolidAbove(const IVector3 &pos) const;
  IVector3 GetRandomTeleportTarget(Random &random, const Vector3 &extents = Vector3(0,0,0)) const;
  IVector3 GetRandomCeiling(Random &random) const;

  void                  TriggerOn               (size_t id);
  void                  TriggerOff              (size_t id);

  void                  MarkForUpdateNeighbours (const Cell *cell);
  void                  UpdateCell              (const IVector3 &pos);

  size_t                GetCellIndex            (const IVector3 &pos) const { return pos.x+size.x*(pos.y+size.y*pos.z); }
  IVector3              GetCellPos              (size_t i)            const { return IVector3( i%size.x, (i/size.x)%size.y, (i/(size.x*size.y))%size.z); }
  bool                  IsValidCellPosition     (const IVector3 &pos) const { return pos.x < size.x  && pos.y < size.y && pos.z < size.z;  }

private:

  RunningState &state;
  IVector3 size;
  bool dirty, firstDirty;

  MiniMap minimap;

  size_t cellCount;
  std::vector<Cell> cells;
  mutable Cell defaultCell;
  std::vector<bool> defaultMask;

  std::vector<size_t> dynamicCells;
  float nextTickT;
  float tickInterval;

  std::unordered_set<size_t> neighbourUpdates;

  IColor ambientLight;

  VertexBuffer allVerts;
  std::unordered_map<const Texture *, size_t> vertexStartsNormal;
  std::unordered_map<const Texture *, size_t> vertexCountsNormal;
  std::unordered_map<const Texture *, size_t> vertexStartsEmissive;
  std::unordered_map<const Texture *, size_t> vertexCountsEmissive;

  bool checkOverwrite;
  bool checkOverwriteOK;

  void UpdateCell(size_t i);
  void MarkForUpdateNeighbours(size_t i);

  friend Serializer &operator << (Serializer &ser, const World &world);
};

inline Cell &
World::GetCell(const IVector3 &pos) const {
  if (checkOverwrite) return const_cast<Cell &>(this->defaultCell);
  if (!this->IsValidCellPosition(pos)) return const_cast<Cell &>(this->defaultCell);
  return const_cast<Cell &>(this->cells[this->GetCellIndex(pos)]);
}

#endif

