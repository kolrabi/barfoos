#ifndef BARFOOS_PLAYER_H
#define BARFOOS_PLAYER_H

#include "common.h"
#include "mob.h"
#include "item.h"

class Player : public Mob {
public:

  Player();
  virtual ~Player();

  virtual void Update(float t);
  virtual void Draw();
  
  void View();
  void MapView();
  
  void DrawWeapons();
  void DrawGUI();
  
  void MouseClick(const Point &pos, int button, bool down);

  const IColor GetTorchLight();

private:

  void UpdateInput();
  void UpdateSelection();

  float bobPhase;
  float bobAmplitude;

  std::shared_ptr<Entity> selectedEntity;
  Cell *selectedCell;
  Side selectedCellSide;
  float selectionRange;
  
  bool itemActiveLeft, itemActiveRight;
  
  unsigned int crosshairTex;
};



#endif

