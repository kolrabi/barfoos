#ifndef BARFOOS_PLAYER_H
#define BARFOOS_PLAYER_H

#include "common.h"
#include "mob.h"

class Gfx;

class Player : public Mob {
public:

  Player();
  virtual ~Player();

  virtual void Update() override;
  virtual void Draw(Gfx &gfx) const override;
  
  void View(Gfx &gfx) const;
  void MapView(Gfx &gfx) const;
  
  void DrawWeapons(Gfx &gfx) const;
  void DrawGUI(Gfx &gfx) const;
  
  void OnMouseClick(const Point &pos, int button, bool down);
  void OnMouseDelta(const Point &delta);

  const IColor GetTorchLight() const;

private:

  void UpdateInput();
  void UpdateSelection();

  float bobPhase;
  float bobAmplitude;

  size_t selectedEntity;
  Cell *selectedCell;
  Side selectedCellSide;
  float selectionRange;
  
  bool itemActiveLeft, itemActiveRight;
  
  const Texture *crosshairTex;
};



#endif

