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

  Cell &                    operator=(const Cell &that);

  void                      SetWorld(World *world, const IVector3 &pos);
  void                      Update(RunningState &state);

  void                      UpdateNeighbours(size_t depth = 16);

  // TODO: CellRender
  Cell &                    SetOrder(bool topReversed, bool bottomReversed);

  // TODO: CellBase --->
  bool                      IsSeen(size_t checkNeighbours = 0) const;
  // <---

  void                      PlaySound(RunningState &state, const std::string &sound);

  const IColor &            GetLightLevel() const;
  bool                      SetLightLevel(const IColor &level, bool force = false);

  bool                      CheckSideSolid(Side side, const Vector3 &org, bool sneak = false) const;

  AABB                      GetAABB() const;

  void                      Tick(RunningState &state);

  bool                      Ray(const Vector3 &start, const Vector3 &dir, float &t, Vector3 &p) const;

  bool                      Flow(Side side);

  void                      OnUse(RunningState &state, Mob &user, bool force = false);
  void                      OnUseItem(RunningState &state, Mob &user, Item &item);
  void                      OnStepOn(RunningState &state, Mob &mob);
  void                      OnStepOff(RunningState &state, Mob &mob);

  void                      Rotate();

  Cell &                    SetYOffsets(float a, float b, float c, float d);
  Cell &                    SetYOffsetsBottom(float a, float b, float c, float d);

protected:

  // unique information, that will change after assignment from different cell

  /** Current tick phase from 0 to tickInterval. Tick()s on 0. */
  uint8_t                   tickPhase;

  /** Current light color in cell. */
  IColor                    lightLevel;

  /** Time of last usage. */
  float                     lastUseT;

  friend Serializer &       operator << (Serializer &ser, const Cell &cell);
  friend Deserializer &     operator >> (Deserializer &deser, Cell &cell);
};

Serializer &operator << (Serializer &ser, const Cell &);

/** Get the current light level of the cell.
  * @return The light level.
  */
inline const IColor &Cell::GetLightLevel() const {
  return this->lightLevel;
}

/** Update the current light level of the cell.
  * @param level New light level.
  * @param force Force setting the light level.
  * @return true if light level was changed.
  */
inline bool Cell::SetLightLevel(const IColor &level, bool force) {
  // only update when changing
  if (!force && level == this->lightLevel) return false;

  lightLevel = level;
  return true;
}

#endif

