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
  
  void MouseClick(int button, bool down);

  const IColor &GetTorchLight();

private:

  void UpdateInput();
  void UpdateSelection();
  void DrawInventorySlot(Point p, InventorySlot slot);

  float bobPhase;
  float bobAmplitude;

  std::shared_ptr<Entity> selectedEntity;
  Cell *selectedCell;
  Side selectedCellSide;
  float selectionRange;

  IColor torchLight;
  
  bool itemActiveLeft, itemActiveRight;
  
  unsigned int crosshairTex;
  unsigned int slotTex;
  unsigned int slotRightHandTex;
  unsigned int slotLeftHandTex;
};



#endif

