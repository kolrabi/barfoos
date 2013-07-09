#ifndef BARFOOS_PLAYER_H
#define BARFOOS_PLAYER_H

#include "common.h"
#include "mob.h"

struct InputEvent;
class RenderString;
class Shader;

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

  void AddMessage(const std::string &text);

private:

  void UpdateInput(Game &game);
  void UpdateSelection(Game &game);

  struct Message {
    RenderString *text;
    float messageTime;

    Message(const std::string &text);
    ~Message();
  };

  std::list<Message*> messages;
  float messageY, messageVY;

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
  std::unique_ptr<Shader> defaultShader;
  std::unique_ptr<Shader> guiShader;
};



#endif

