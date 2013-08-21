#ifndef BARFOOS_CELLBASE_H
#define BARFOOS_CELLBASE_H

#include "cellproperties.h"

#include "trigger.h"

class CellBase : public Triggerable {
public:

  const std::string &GetType() const;
  const CellProperties &GetInfo() const;

  void SetTeleportTarget(const IVector3 &target);
  void SetTriggerTarget(uint32_t triggerTargetId);

  void SetSpawnOnActive(const std::string &mob, Side side, float rate);

  void Lock(ID id);
  void Unlock();
  ID GetLockedID() const;

  float GetHeight(float x, float z) const;
  float GetHeightBottom(float x, float z) const;
  float GetHeightClamp(float x, float z) const;
  float GetHeightBottomClamp(float x, float z) const;

  bool IsTopFlat() const;
  bool IsBottomFlat() const;
  bool IsTransparent() const;

protected:

  CellBase(const std::string &type);

  const CellProperties *info;
  World *world;
  IVector3 pos;
  Cell *neighbours[6];
  float lastT;
  float nextActivationT;

  bool teleport;
  IVector3 teleportTarget;

  std::string spawnOnActiveMob;
  SpawnClass spawnOnActiveClass;
  Side spawnOnActiveSide;
  float spawnOnActiveRate;

  bool isTrigger;
  ID triggerTargetId;

  static const int OffsetScale = 127;

  // shared information, that will stay the same after assignment from different cell
  struct SharedInfo {
    uint8_t tickInterval;

    // map generation
    bool isLocked;    // disallow modification via World::SetCell...
    bool ignoreLock;  // ..unless this is true
    bool ignoreWrite; // World::SetCell will only pretend to succeed
    ID featureID;

    // rendering
    bool reversedTop;
    bool reversedBottom;

    int16_t topHeights[4], bottomHeights[4];
    float u[4], v[4];

    uint32_t detail;
    float smoothDetail;

    ID lockedID;
    Vector3 scale;

    bool topFlat, bottomFlat;

    SharedInfo(const CellProperties *info) :
      tickInterval( info->flags & CellFlags::Viscous ? 32 : 5 ),
      isLocked(false),
      ignoreLock(false),
      ignoreWrite(false),
      featureID(InvalidID),
      reversedTop(false),
      reversedBottom(false),
      topHeights { OffsetScale, OffsetScale, OffsetScale, OffsetScale },
      bottomHeights { 0, 0, 0, 0 },
      u { 0,0,0,0 },
      v { 0,0,0,0 },
      detail( info->flags & CellFlags::Liquid ? 15 : 0 ),
      smoothDetail( detail ),
      lockedID(0),
      scale(info->scale),
      topFlat(true),
      bottomFlat(true)
    { }
  } shared;

  float YOfs(size_t n)  const { return this->shared.topHeights[n]/(float)OffsetScale; }
  float YOfsb(size_t n) const { return this->shared.bottomHeights[n]/(float)OffsetScale; }

  bool vertsDirty, colorDirty;
};

inline const std::string &CellBase::GetType() const {
  return this->info->type;
}

inline const CellProperties &CellBase::GetInfo() const {
  return *this->info;
}

inline void CellBase::SetTeleportTarget(const IVector3 &target) {
  this->teleport = true;
  this->teleportTarget = target;
}

inline void CellBase::SetTriggerTarget(ID id) {
  this->isTrigger = id != 0 && id != InvalidID;
  this->triggerTargetId = id;
}

inline void CellBase::SetSpawnOnActive(const std::string &mob, Side side, float rate) {
  this->spawnOnActiveMob = mob;
  this->spawnOnActiveSide = side;
  this->spawnOnActiveRate = rate;
}

inline uint32_t CellBase::GetLockedID() const {
  return this->shared.lockedID;
}

inline float
CellBase::GetHeight(
  float x,
  float z
) const {
  x -= (int)x;
  z -= (int)z;

  return GetHeightClamp(x, z);
}

inline float
CellBase::GetHeightClamp(
  float x,
  float z
) const {
  if (this->IsTopFlat()) return YOfs(0);

  if (x > 1.0) x = 1.0;
  if (x < 0.0) x = 0.0;
  if (z > 1.0) z = 1.0;
  if (z < 0.0) z = 0.0;

  float slopeX;
  float slopeZ;

  if (this->shared.reversedTop) {
    // Z ^
    //   | 1---2
    //   | | \ |
    //   | 0---3
    //   +------> X

    if (x < z) {
      // lower left triangle
      slopeX = YOfs(3) - YOfs(0);
      slopeZ = YOfs(1) - YOfs(0);
    } else {
      // upper right triangle
      slopeX = YOfs(2) - YOfs(1);
      slopeZ = YOfs(2) - YOfs(3);
    }
  } else {
    // Z ^
    //   | 1---2
    //   | | / |
    //   | 0---3
    //   +------> X

    if (x < z) {
      // upper left triangle
      slopeX = YOfs(2) - YOfs(1);
      slopeZ = YOfs(1) - YOfs(0);
    } else {
      // lower right triangle
      slopeX = YOfs(3) - YOfs(0);
      slopeZ = YOfs(2) - YOfs(3);
    }
  }

  return YOfs(0) + slopeX * x + slopeZ * z;
}

inline float
CellBase::GetHeightBottom(
  float x,
  float z
) const {
  x -= (int)x;
  z -= (int)z;

  return GetHeightBottomClamp(x,z);
}

inline float
CellBase::GetHeightBottomClamp(
  float x,
  float z
) const {
  if (this->IsBottomFlat()) return YOfsb(0);

  if (x > 1.0) x = 1.0;
  if (x < 0.0) x = 0.0;
  if (z > 1.0) z = 1.0;
  if (z < 0.0) z = 0.0;

  float slopeX;
  float slopeZ;

  if (this->shared.reversedBottom) {
    // Z ^
    //   | 1---2
    //   | | \ |
    //   | 0---3
    //   +------> X

    if (x < z) {
      // lower left triangle
      slopeX = YOfsb(3) - YOfsb(0);
      slopeZ = YOfsb(1) - YOfsb(0);
    } else {
      // upper right triangle
      slopeX = YOfsb(2) - YOfsb(1);
      slopeZ = YOfsb(2) - YOfsb(3);
    }
  } else {
    // Z ^
    //   | 1---2
    //   | | / |
    //   | 0---3
    //   +------> X

    if (x < z) {
      // upper left triangle
      slopeX = YOfsb(2) - YOfsb(1);
      slopeZ = YOfsb(1) - YOfsb(0);
    } else {
      // lower right triangle
      slopeX = YOfsb(3) - YOfsb(0);
      slopeZ = YOfsb(2) - YOfsb(3);
    }
  }

  return YOfsb(0) + slopeX * x + slopeZ * z;
}

inline bool CellBase::IsTopFlat() const {
  return this->shared.topFlat; //YOfs(0) == 1.0 && YOfs(1) == 1.0 && YOfs(2) == 1.0 && YOfs(3) == 1.0;
}

inline bool CellBase::IsBottomFlat() const {
  return this->shared.bottomFlat; //YOfsb(0) == 0.0 && YOfsb(1) == 0.0 && YOfsb(2) == 0.0 && YOfsb(3) == 0.0;
}

inline bool CellBase::IsTransparent() const {
  return
    this->GetInfo().flags & CellFlags::Transparent ||
    this->GetInfo().flags & CellFlags::DoNotRender ||
    !IsTopFlat() || !IsBottomFlat() || YOfsb(0) != 0.0 || YOfs(0) != 1.0;
}


#endif

