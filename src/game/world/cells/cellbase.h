#ifndef BARFOOS_CELLBASE_H
#define BARFOOS_CELLBASE_H

#include "game/world/cells/cellproperties.h"
#include "game/gameplay/trigger.h"

#include "world.pb.h"

class CellBase : public Triggerable {
public:

  const std::string &       GetType() const { return this->proto.type(); }
  const CellProperties &    GetInfo() const { return *this->info; }
  World *                   GetWorld() const { return this->world; }
  const IVector3 &          GetPosition() const { return this->pos; }

  bool                      IsSeen(size_t checkNeighbours = 0) const;

  void                      SetLockID(ID id) { this->proto.set_lock_id(id); }
  void                      ClearLockID() { this->proto.clear_lock_id(); }
  ID                        GetLockID() const { return this->proto.lock_id(); }
  bool                      IsLocked() const { return this->proto.has_lock_id(); }

  float                     GetNextActivationTime() const { return this->proto.next_activation_time(); }
  void                      SetNextActivationTime(float f) { this->proto.set_next_activation_time(f); }

  void                      SetTeleportTarget(uint32_t index) { this->proto.set_teleport_target(index); }
  uint32_t                  GetTeleportTarget() const { return this->proto.teleport_target(); }
  bool                      IsTeleport() const { return this->proto.has_teleport_target(); }

  void                      SetTriggerTarget(uint32_t triggerTargetId) { this->proto.set_trigger_target_id(triggerTargetId); }
  ID                        GetTriggerTarget() const { return this->IsTrigger() ? this->proto.trigger_target_id() : InvalidID; }
  bool                      IsTrigger() const { return this->proto.has_trigger_target_id(); }

  void                      SetProtected(bool locked) { this->proto.set_is_protected(locked); }
  bool                      IsProtected() const { return this->proto.is_protected(); }

  void                      SetIgnoringProtection(bool ignore) { this->proto.set_is_ignoring_protection(ignore); }
  bool                      IsIgnoringProtection() const { return this->proto.is_ignoring_protection(); }

  void                      SetIgnoringWrite(bool ignore) { this->proto.set_is_ignoring_write(ignore); }
  bool                      IsIgnoringWrite() const { return this->proto.is_ignoring_write(); }

  void                      SetFeatureID(ID id) { this->proto.set_feature_id(id); }
  ID                        GetFeatureID() const { return this->proto.has_feature_id() ? this->proto.feature_id() : InvalidID; }

  bool                      IsTopReversed() const { return this->proto.is_top_reversed(); }
  bool                      IsBottomReversed() const { return this->proto.is_bottom_reversed(); }
  void                      SetReversed(bool top, bool bottom);

  void                      SetSideReversed     (bool rev)                                      { this->proto.set_is_side_reversed(rev); }
  bool                      IsSideReversed      ()                                        const { return this->proto.is_side_reversed(); }

  bool                      HasSpawnOnActive() const { return this->proto.has_spawn_on_active(); }
  void                      SetSpawnOnActive(const std::string &mob, Side side, float rate) {
                              this->proto.mutable_spawn_on_active()->set_mob_type(mob);
                              this->proto.mutable_spawn_on_active()->set_side(uint32_t(side));
                              this->proto.mutable_spawn_on_active()->set_rate(rate);
                            }

  const std::string &       GetSpawnOnActiveMobType() const { return this->proto.spawn_on_active().mob_type(); }
  Side                      GetSpawnOnActiveSide() const { return Side(this->proto.spawn_on_active().side()); }
  float                     GetSpawnOnActiveRate() const { return this->proto.spawn_on_active().rate(); }

  uint32_t                  GetLiquidAmount() const { return this->proto.liquid_amount(); }
  void                      SetLiquidAmount(uint32_t amt) { this->proto.set_liquid_amount(amt); }

  void                      SetTopHeight(size_t n, float f) { 
                              switch(n) {
                                case 0:  this->proto.mutable_top_heights()->set_a(f); break;
                                case 1:  this->proto.mutable_top_heights()->set_b(f); break;
                                case 2:  this->proto.mutable_top_heights()->set_c(f); break;
                                case 3:  this->proto.mutable_top_heights()->set_d(f); break;
                              }
                            }
                            
  float                     GetTopHeight(size_t n) const { 
                              switch(n) {
                                case 0:  return this->proto.top_heights().a();
                                case 1:  return this->proto.top_heights().b();
                                case 2:  return this->proto.top_heights().c();
                                case 3:  return this->proto.top_heights().d();
                                default: return 1.0f;
                              }
                            }

  void                      SetTopHeights(float a, float b, float c, float d);
  void                      SetBottomHeights(float a, float b, float c, float d);

  bool                      IsTopFlat() const { return !this->proto.has_top_heights(); }

  void                      SetBottomHeight(size_t n, float f) { 
                              switch(n) {
                                case 0:  this->proto.mutable_bottom_heights()->set_a(f); break;
                                case 1:  this->proto.mutable_bottom_heights()->set_b(f); break;
                                case 2:  this->proto.mutable_bottom_heights()->set_c(f); break;
                                case 3:  this->proto.mutable_bottom_heights()->set_d(f); break;
                              }
                            }
                            
  float                     GetBottomHeight(size_t n) const { 
                              switch(n) {
                                case 0:  return this->proto.bottom_heights().a();
                                case 1:  return this->proto.bottom_heights().b();
                                case 2:  return this->proto.bottom_heights().c();
                                case 3:  return this->proto.bottom_heights().d();
                                default: return 1.0f;
                              }
                            }

  bool                      IsBottomFlat() const { return !this->proto.has_top_heights(); }

  void                      SetScale(const Vector3 &scale) { this->proto.set_scale_x(scale.x); this->proto.set_scale_y(scale.y); this->proto.set_scale_z(scale.z); }
  Vector3                   GetScale() const { return Vector3( this->proto.scale_x(), this->proto.scale_y(), this->proto.scale_z() ); }

  void                      SetLastUseTime(float f) { this->proto.set_last_use_time(f); }
  float                     GetLastUseTime() const { return this->proto.last_use_time(); }

  IColor                    GetLightLevel() const { return IColor(this->proto.light_r(), this->proto.light_g(), this->proto.light_b()); }
  bool                      SetLightLevel(const IColor &level, bool force = false) {
                              // only update when changing
                              if (!force && level == this->GetLightLevel()) return false;
                              this->proto.set_light_r(level.r);
                              this->proto.set_light_g(level.g);
                              this->proto.set_light_b(level.b);
                              return true;
                            }

  virtual void              SetTrigger(uint32_t id, bool toggle = false) override { this->proto.set_trigger_id(id); this->proto.set_is_trigger_toggle(toggle); }
  virtual uint32_t          GetTriggerId() const override { return this->proto.has_trigger_id() ? this->proto.trigger_id() : InvalidID; }
  virtual bool              IsTriggerToggle() const override { return this->proto.is_trigger_toggle(); }

  virtual void              SetTriggered(bool triggered) override { this->proto.set_is_triggered(triggered); }
  virtual bool              IsTriggered() const override{ return this->proto.is_triggered(); }

  // -------------------------------------------------------------

  void                      Lock(ID id);
  void                      Unlock();

  float                     GetHeight(float x, float z) const;
  float                     GetHeightBottom(float x, float z) const;
  float                     GetHeightClamp(float x, float z) const;
  float                     GetHeightBottomClamp(float x, float z) const;

  bool                      IsDynamic() const;
  bool                      IsLiquid() const;
  bool                      IsSolid() const;
  bool                      IsTransparent() const;

  Cell &                    operator[](Side side);
  const Cell &              operator[](Side side) const;

protected:

                            CellBase(const std::string &type);
                            CellBase(const CellBase &that);
                            CellBase(const Cell_Proto &proto);

  const CellProperties *    info;
  World *                   world;
  IVector3                  pos;
  Cell *                    neighbours[6] = {};

  uint8_t                   tickInterval;

  Cell_Proto proto;

  bool vertsDirty, colorDirty;
};

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
  if (this->IsTopFlat()) return GetTopHeight(0);

  if (x > 1.0) x = 1.0;
  if (x < 0.0) x = 0.0;
  if (z > 1.0) z = 1.0;
  if (z < 0.0) z = 0.0;

  float slopeX;
  float slopeZ;

  if (this->IsTopReversed()) {
    // Z ^
    //   | 1---2
    //   | | \ |
    //   | 0---3
    //   +------> X

    if (x < z) {
      // lower left triangle
      slopeX = GetTopHeight(3) - GetTopHeight(0);
      slopeZ = GetTopHeight(1) - GetTopHeight(0);
    } else {
      // upper right triangle
      slopeX = GetTopHeight(2) - GetTopHeight(1);
      slopeZ = GetTopHeight(2) - GetTopHeight(3);
    }
  } else {
    // Z ^
    //   | 1---2
    //   | | / |
    //   | 0---3
    //   +------> X

    if (x < z) {
      // upper left triangle
      slopeX = GetTopHeight(2) - GetTopHeight(1);
      slopeZ = GetTopHeight(1) - GetTopHeight(0);
    } else {
      // lower right triangle
      slopeX = GetTopHeight(3) - GetTopHeight(0);
      slopeZ = GetTopHeight(2) - GetTopHeight(3);
    }
  }

  return GetTopHeight(0) + slopeX * x + slopeZ * z;
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
  if (this->IsBottomFlat()) return GetBottomHeight(0);

  if (x > 1.0) x = 1.0;
  if (x < 0.0) x = 0.0;
  if (z > 1.0) z = 1.0;
  if (z < 0.0) z = 0.0;

  float slopeX;
  float slopeZ;

  if (this->IsBottomReversed()) {
    // Z ^
    //   | 1---2
    //   | | \ |
    //   | 0---3
    //   +------> X

    if (x < z) {
      // lower left triangle
      slopeX = GetBottomHeight(3) - GetBottomHeight(0);
      slopeZ = GetBottomHeight(1) - GetBottomHeight(0);
    } else {
      // upper right triangle
      slopeX = GetBottomHeight(2) - GetBottomHeight(1);
      slopeZ = GetBottomHeight(2) - GetBottomHeight(3);
    }
  } else {
    // Z ^
    //   | 1---2
    //   | | / |
    //   | 0---3
    //   +------> X

    if (x < z) {
      // upper left triangle
      slopeX = GetBottomHeight(2) - GetBottomHeight(1);
      slopeZ = GetBottomHeight(1) - GetBottomHeight(0);
    } else {
      // lower right triangle
      slopeX = GetBottomHeight(3) - GetBottomHeight(0);
      slopeZ = GetBottomHeight(2) - GetBottomHeight(3);
    }
  }

  return GetBottomHeight(0) + slopeX * x + slopeZ * z;
}

inline bool CellBase::IsTransparent() const {
  return
    this->GetInfo().flags & CellFlags::Transparent ||
    this->GetInfo().flags & CellFlags::DoNotRender ||
    GetBottomHeight(0) != 0.0 || 
    GetBottomHeight(1) != 0.0 || 
    GetBottomHeight(2) != 0.0 || 
    GetBottomHeight(3) != 0.0 || 
    GetTopHeight(0) != 1.0 ||
    GetTopHeight(1) != 1.0 ||
    GetTopHeight(2) != 1.0 ||
    GetTopHeight(3) != 1.0 
    ;
}

inline bool CellBase::IsSolid() const {
  return this->info->flags & CellFlags::Solid;
}

inline bool CellBase::IsLiquid() const {
  return this->info->flags & CellFlags::Liquid;
}

inline bool CellBase::IsDynamic() const {
  return this->info->flags & CellFlags::Dynamic || this->GetTriggerId() || this->IsTrigger();
}

inline Cell &CellBase::operator[](Side side) {
  return *this->neighbours[(int)side];
}

inline const Cell &CellBase::operator[](Side side) const {
  return *this->neighbours[(int)side];
}

#endif

