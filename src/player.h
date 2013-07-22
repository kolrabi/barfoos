#ifndef BARFOOS_PLAYER_H
#define BARFOOS_PLAYER_H

#include "common.h"
#include "mob.h"

#include <unordered_map>
#include <list>

class Player final : public Mob {
public:

  Player();
  Player(const Player &) = delete;
  virtual ~Player();

  Player &operator=(const Player &) = delete;

  virtual void Start(Game &game, size_t id) override;
  virtual void Update(Game &game) override;
  virtual void Draw(Gfx &gfx) const override;

  virtual void AddHealth(Game &game, const HealthInfo &info) override; 
  
  void View(Gfx &gfx) const;
  void MapView(Gfx &gfx) const;
  
  void DrawWeapons(Gfx &gfx) const;
  void DrawGUI(Gfx &gfx) const;
  
  void HandleEvent(const InputEvent &event);

  void AddMessage(const std::string &text, const std::string &font = "default");
  void AddDeathMessage(const Entity &dead, const HealthInfo &info);
  void AddDeathMessage(const Entity &dead, const Entity &killer, const HealthInfo &info);
  
  virtual void OnHealthDealt(Game &game, Entity &other, const HealthInfo &info) override;

  virtual std::string       GetName()                         const;
  float                     GetPain()                         const { return this->pain; }
  
  void SetUniforms(const Shader *shader) const;
  
private:

  struct Message {
    RenderString *text;
    float messageTime;

    Message(const std::string &text, const std::string &font);
    Message(const Message &) = default;
    ~Message();
    
    Message &operator=(const Message &) = default;
  };

  // rendering
  const Texture *crosshairTex;
  const Texture *slotTex;
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
  
  std::unordered_map<HealthType, float> lastHurtT;
  float pain;

  // display
  std::list<Message*> messages;
  float messageY, messageVY;
  float fps;
  
  std::shared_ptr<Item> leftHand, rightHand;

  void UpdateInput(Game &game);
  void UpdateSelection(Game &game);
};



#endif

