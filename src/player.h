#ifndef BARFOOS_PLAYER_H
#define BARFOOS_PLAYER_H


#include "common.h"
#include "mob.h"

class World;
class Item;

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

private:

  void UpdateInput();
  void DrawInventorySlot(float x, float y, size_t slot);

  float bobPhase;
  float bobAmplitude;

  std::shared_ptr<Entity> selectedMob;
  
  bool itemActiveLeft, itemActiveRight;
  
  unsigned int crosshairTex;
  unsigned int slotTex;
  unsigned int slotRightHandTex;
  unsigned int slotLeftHandTex;
};



#endif

