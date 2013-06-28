#ifndef BARFOOS_PLAYER_H
#define BARFOOS_PLAYER_H

#include "common.h"
#include "mob.h"
#include "item.h"

class Player : public Mob {
public:

  Player();
  virtual ~Player();

  virtual void Update();
  virtual void Draw() const;
  
  void View() const;
  void MapView() const;
  
  void DrawWeapons() const;
  void DrawGUI() const;
  
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

