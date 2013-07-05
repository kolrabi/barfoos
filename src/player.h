#ifndef BARFOOS_PLAYER_H
#define BARFOOS_PLAYER_H

#include "common.h"
#include "mob.h"

struct InputEvent;

class Player : public Mob {
public:

  Player();
  virtual ~Player();

  virtual void Update(Game &game) override;
  virtual void Draw(Gfx &gfx) const override;
  
  void View(Gfx &gfx) const;
  void MapView(Gfx &gfx) const;
  
  void DrawWeapons(Gfx &gfx) const;
  void DrawGUI(Gfx &gfx) const;
  
  void HandleEvent(const InputEvent &event);

private:

  void UpdateInput(Game &game);
  void UpdateSelection(Game &game);

  float fps;
  float bobPhase;
  float bobAmplitude;

  size_t selectedEntity;
  Cell *selectedCell;
  Side selectedCellSide;
  float selectionRange;

  IColor torchLight;
  
  bool itemActiveLeft, itemActiveRight;
  
  const Texture *crosshairTex;
};



#endif

