#ifndef BARFOOS_WORLD_H
#define BARFOOS_WORLD_H

#include "cell.h"
#include "icolor.h"
#include "vertexbuffer.h"

#include <unordered_map>
#include <unordered_set>

#include "world.pb.h"

class MiniMap final {
public:

  MiniMap(const World &world);
  MiniMap(const World &world, const MiniMap_Proto &proto);

  void Draw(Gfx &gfx, const Vector3 &eyePos, float angle);

  void AddFeatureSeen(ID f);
  bool IsFeatureSeen(ID id) const;

  const MiniMap_Proto &GetProto();

private:

  const World &world;
  MiniMap_Proto proto;

  std::vector<bool> seenFeatures;
  const Texture *mapTexture;

  size_t viewY;
  void RepaintMap();
};

class World final {
public:

  World(RunningState &state, const IVector3 &size);
  World(RunningState &state, const World_Proto &proto);
  World(const World &world) = delete;
  World(World &&world) = delete;
  ~World();

  World &operator=(const World &) = delete;

  RunningState &  GetState()  const { return state; }
  MiniMap &       GetMap()          { return minimap; }

  IVector3  GetSize()   const { return IVector3(this->proto.size_x(), this->proto.size_y(), this->proto.size_z()); }
  size_t    GetCellCount() const { return this->cells.size(); }

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

  size_t                GetCellIndex            (const IVector3 &pos) const { return pos.x+proto.size_x()*(pos.y+proto.size_y()*pos.z); }
  IVector3              GetCellPos              (size_t i)            const { return IVector3( i%proto.size_x(), (i/proto.size_x())%proto.size_y(), (i/(proto.size_x()*proto.size_y()))%proto.size_z()); }
  bool                  IsValidCellPosition     (const IVector3 &pos) const { return pos.x < proto.size_x() && pos.y < proto.size_y() && pos.z < proto.size_z();  }

  const World_Proto &   GetProto();

private:

  static constexpr float tickInterval = 0.1f;
  static const IColor ambientLight;

  float                 GetNextTickTime         ()                    const { return this->proto.next_tick_time(); }
  void                  SetNextTickTime         (float t)                   { this->proto.set_next_tick_time(t); }

  RunningState &state;
  World_Proto proto;
  MiniMap minimap;

  bool dirty, firstDirty;

  std::vector<Cell> cells;
  std::vector<bool> defaultMask;

  mutable Cell defaultCell;

  std::vector<size_t> dynamicCells;

  std::unordered_set<size_t> neighbourUpdates;

  VertexBuffer allVerts;
  std::unordered_map<const Texture *, size_t> vertexStartsNormal;
  std::unordered_map<const Texture *, size_t> vertexCountsNormal;
  std::unordered_map<const Texture *, size_t> vertexStartsEmissive;
  std::unordered_map<const Texture *, size_t> vertexCountsEmissive;

  bool checkOverwrite;
  bool checkOverwriteOK;

  void UpdateCell(size_t i);
  void MarkForUpdateNeighbours(size_t i);

};

inline Cell &
World::GetCell(const IVector3 &pos) const {
  if (checkOverwrite) return const_cast<Cell &>(this->defaultCell);
  if (!this->IsValidCellPosition(pos)) return const_cast<Cell &>(this->defaultCell);
  return const_cast<Cell &>(this->cells[this->GetCellIndex(pos)]);
}

#endif

