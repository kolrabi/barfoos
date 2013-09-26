#ifndef BARFOOS_CELL_H
#define BARFOOS_CELL_H

#include "cellrender.h"

#include "world.pb.h"

/** A cell in the world. */
class Cell final : public CellRender {
public:

                            Cell(const std::string &type = "default");
                            Cell(const Cell &that);
                            Cell(const Cell_Proto &proto);
                            ~Cell();

  void                      SetWorld(World *world, const IVector3 &pos);
  void                      Update(RunningState &state);

  void                      UpdateNeighbours(size_t depth = 16);

  void                      PlaySound(RunningState &state, const std::string &sound);

  AABB                      GetAABB() const;
  bool                      CheckSideSolid(Side side, const Vector3 &org, bool sneak = false) const;
  bool                      Ray(const Vector3 &start, const Vector3 &dir, float &t, Vector3 &p) const;

  void                      Tick(RunningState &state);
  bool                      Flow(Side side);

  void                      OnUse(RunningState &state, Mob &user, bool force = false);
  void                      OnUseItem(RunningState &state, Mob &user, Item &item);
  void                      OnStepOn(RunningState &state, Mob &mob);
  void                      OnStepOff(RunningState &state, Mob &mob);

  void                      Rotate();

protected:

  /** Current tick phase from 0 to tickInterval. Tick()s on 0. */
  uint8_t                   tickPhase;
};

#endif

