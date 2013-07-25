#ifndef BARFOOS_PLAYER_H
#define BARFOOS_PLAYER_H

#include "common.h"
#include "mob.h"

#include <unordered_map>
#include <list>

class Player final : public Mob {
public:

  Player();
  Player(Deserializer &deser);
  Player(const Player &) = delete;
  virtual ~Player();

  Player &operator=(const Player &) = delete;

  virtual void Start(RunningState &state, size_t id) override;
  virtual void Update(RunningState &state) override;
  virtual void Draw(Gfx &gfx) const override;

  virtual void AddHealth(RunningState &state, const HealthInfo &info) override; 
  
  void View(Gfx &gfx) const;
  void MapView(Gfx &gfx) const;
  
  void DrawWeapons(Gfx &gfx) const;
  void DrawGUI(Gfx &gfx) const;
  
  void HandleEvent(const InputEvent &event);

  void AddMessage(const std::string &text, const std::string &font = "default");
  void AddDeathMessage(const Entity &dead, const HealthInfo &info);
  void AddDeathMessage(const Entity &dead, const Entity &killer, const HealthInfo &info);
  
  virtual void OnHealthDealt(RunningState &state, Entity &other, const HealthInfo &info) override;
  virtual void OnEquip(RunningState &, const Item &, InventorySlot, bool) override;

  virtual std::string       GetName()                         const;
  float                     GetPain()                         const { return this->pain; }
  
  void SetUniforms(const std::shared_ptr<Shader> &shader) const;
  
  virtual void              Serialize(Serializer &ser)        const;
  
private:

  struct Message {
    RenderString *text;
    float messageTime;

    Message(const std::string &text, const std::string &font);
    Message(Message &&other) {
      this->text = other.text;
      this->messageTime = other.messageTime;
      other.text = nullptr;
    }
    
    ~Message();
    
    Message &operator=(const Message &) = default;
  };
  
  friend Serializer &operator << (Serializer &ser, const Message *msg);
  friend Deserializer &operator >> (Deserializer &ser, Message *&msg);

  // rendering
  const Texture *crosshairTex;
  const Texture *slotTex;
  float bobPhase;
  float bobAmplitude;
  
  // gameplay
  size_t selectedEntity;
  Cell *selectedCell;
  Side selectedCellSide;
  float selectionRange;

  bool itemActiveLeft, itemActiveRight;
  
  std::unordered_map<size_t, float> lastHurtT;
  float pain;

  // display
  std::list<Message*> messages;
  float messageY, messageVY;
  float fps;
  
  std::shared_ptr<Item> leftHand, rightHand;

  void UpdateInput(RunningState &state);
  void UpdateSelection(RunningState &state);
  
  virtual SpawnClass GetSpawnClass() const override { return SpawnClass::PlayerClass; }
};

#endif
