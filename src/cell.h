#ifndef BARFOOS_CELL_H
#define BARFOOS_CELL_H

#include "common.h"
#include "icolor.h"
#include "ivector3.h"

#include "cellrender.h"

/** A cell in the world. */
class Cell final : public CellRender {
public:

  Cell(const std::string &type = "default");
  Cell(const Cell &that);
  ~Cell();

  Cell &operator=(const Cell &that);

  void Update(RunningState &state);

  void UpdateNeighbours(size_t depth = 16);

  uint8_t GetVisibility() const;
  void SetVisibility(uint8_t visibility);

  Cell &SetOrder(bool topReversed, bool bottomReversed);

  bool IsSolid() const;
  bool IsLiquid() const;
  bool IsDynamic() const;
  bool IsTrigger() const;

  void SetWorld(World *world, const IVector3 &pos);
  World *GetWorld() const;
  IVector3 GetPosition() const;

  void                      PlaySound(RunningState &state, const std::string &sound);

  const IColor &GetLightLevel() const;
  bool SetLightLevel(const IColor &level, bool force=false);

  bool IsLocked() const;
  void SetLocked(bool locked);

  bool GetIgnoreLock() const;
  void SetIgnoreLock(bool ignore);

  bool GetIgnoreWrite() const;
  void SetIgnoreWrite(bool ignore);

  uint32_t GetDetail() const;
  void SetDetail(uint32_t detail);

  uint32_t GetFeatureID() const;
  void SetFeatureID(uint32_t f);

  bool IsSeen(size_t checkNeighbours = 0) const;

  bool HasSolidSides() const;
  bool CheckSideSolid(Side side, const Vector3 &org, bool sneak = false) const;

  AABB GetAABB() const;

  void Tick(RunningState &state);

  bool Ray(const Vector3 &start, const Vector3 &dir, float &t, Vector3 &p) const;
  Cell &operator[](Side side);
  const Cell &operator[](Side side) const;

  bool Flow(Side side);

  void OnUse(RunningState &state, Mob &user, bool force = false);
  void OnUseItem(RunningState &state, Mob &user, Item &item);
  void OnStepOn(RunningState &state, Mob &mob);
  void OnStepOff(RunningState &state, Mob &mob);

  void Rotate();

  Cell &SetYOffsets(float a, float b, float c, float d);
  Cell &SetYOffsetsBottom(float a, float b, float c, float d);

protected:

  // unique information, that will change after assignment from different cell

  uint8_t tickPhase;
  IColor lightLevel;

  float lastUseT;

  friend Serializer &operator << (Serializer &ser, const Cell &cell);
  friend Deserializer &operator >> (Deserializer &deser, Cell &cell);
};

Serializer &operator << (Serializer &ser, const Cell &);

inline World *Cell::GetWorld() const {
  return this->world;
}

inline IVector3 Cell::GetPosition() const {
  return this->pos;
}

inline uint8_t Cell::GetVisibility() const {
  return this->visibility;
}

inline void Cell::SetVisibility(uint8_t visibility) {
  this->visibility = visibility;
}

inline bool Cell::IsSolid() const {
  return this->info->flags & CellFlags::Solid;
}

inline bool Cell::IsLiquid() const {
  return this->info->flags & CellFlags::Liquid;
}

inline bool Cell::IsDynamic() const {
  return this->info->flags & CellFlags::Dynamic || this->GetTriggerId() || this->IsTrigger();
}

inline bool Cell::IsTrigger() const {
  return this->isTrigger;
}

inline const IColor &Cell::GetLightLevel() const {
  return this->lightLevel;
}

inline bool Cell::SetLightLevel(const IColor &level, bool force) {
  // only update when changing
  if (!force && level == this->lightLevel) return false;

  lightLevel = level;
  return true;
}

inline bool Cell::IsLocked() const {
  return this->shared.isLocked;
}

inline void Cell::SetLocked(bool locked) {
  this->shared.isLocked = locked;
}

inline bool Cell::GetIgnoreLock() const {
  return this->shared.ignoreLock;
}

inline void Cell::SetIgnoreLock(bool ignore) {
  this->shared.ignoreLock = ignore;
}

inline bool Cell::GetIgnoreWrite() const {
  return this->shared.ignoreWrite;
}

inline void Cell::SetIgnoreWrite(bool ignore) {
  this->shared.ignoreWrite = ignore;
}

inline uint32_t Cell::GetDetail() const {
  return this->shared.detail;
}

inline void Cell::SetDetail(uint32_t detail) {
  this->shared.detail = detail;
}

inline uint32_t Cell::GetFeatureID() const {
  return this->shared.featureID;
}

inline void Cell::SetFeatureID(uint32_t f) {
  this->shared.featureID = f;
}

inline Cell &Cell::operator[](Side side) {
  return *this->neighbours[(int)side];
}

inline const Cell &Cell::operator[](Side side) const {
  return *this->neighbours[(int)side];
}


#endif

