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

  virtual void AddHealth(Game &game, const HealthInfo &info) override; 
  
  void View(Gfx &gfx) const;
  void MapView(Gfx &gfx) const;
  
  void DrawWeapons(Gfx &gfx) const;
  void DrawGUI(Gfx &gfx) const;
  
  void HandleEvent(const InputEvent &event);

  void AddMessage(const std::string &text, const std::string &font = "default");
  
private:

  struct Message {
    RenderString *text;
    float messageTime;

    Message(const std::string &text, const std::string &font);
    ~Message();
  };

  // rendering
  const Texture *crosshairTex;
  std::unique_ptr<Shader> defaultShader;
  std::unique_ptr<Shader> guiShader;
  float bobPhase;
  float bobAmplitude;
  
  // gameplay
  size_t selectedEntity;
  Cell *selectedCell;
  Side selectedCellSide;
  float selectionRange;

  bool itemActiveLeft, itemActiveRight;

  // display
  std::list<Message*> messages;
  float messageY, messageVY;
  float fps;

  void UpdateInput(Game &game);
  void UpdateSelection(Game &game);
};



#endif

