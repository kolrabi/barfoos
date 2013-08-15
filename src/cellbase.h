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
      lockedID(0)
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


#endif

