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

  virtual void Start(RunningState &state, ID id) override;
  virtual void Update(RunningState &state) override;
  virtual void Draw(Gfx &gfx) const override;

  virtual void AddHealth(RunningState &state, const HealthInfo &info) override;

  void View(Gfx &gfx) const;
  void MapView(Gfx &gfx) const;

  void DrawWeapons(Gfx &gfx) const;
  void DrawGUI(Gfx &gfx) const;

  void HandleEvent(const InputEvent &event);

  void AddMessage(const std::string &text, const std::string &font = "small");
  void AddDeathMessage(const Entity &dead, const HealthInfo &info);
  void AddDeathMessage(const Entity &dead, const Entity &killer, const HealthInfo &info);

  virtual void OnHealthDealt(RunningState &state, Entity &other, const HealthInfo &info) override;
  virtual void OnEquip(RunningState &, const Item &, InventorySlot, bool) override;
  virtual void OnLevelUp(RunningState &) override;
  virtual void OnBuffAdded(RunningState &, const EffectProperties &) override;
  virtual void              OnCollide(RunningState &, Entity &) override;
  virtual void              LearnSpell(const std::string &name) override;

  virtual std::string       GetName()                         const;
  float                     GetPain()                         const { return this->pain; }

  void                      SetUniforms(const std::shared_ptr<Shader> &shader) const;

  void                      SetAngles(const Vector3 &angles)        { this->angles = angles; }
  const Vector3 &           GetAngles()                       const { return this->angles; }

  virtual void              Serialize(Serializer &ser)        const;
  virtual SpawnClass        GetSpawnClass()                   const override { return SpawnClass::PlayerClass; }

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

  std::unordered_map<Element, Sprite> gemSprites;

  // gameplay
  bool itemActiveLeft, itemActiveRight;
  bool lastItemActiveLeft, lastItemActiveRight;
  Vector3 angles;

  std::unordered_map<uint32_t, float> lastHurtT;
  float pain;
  float hpFlashT;

  std::vector<Element> elements;

  // display
  std::list<Message*> messages;
  float messageY, messageVY;
  float fps;
  RenderString *bigMessage;
  float bigMessageT;

  float mapZoom;

  std::shared_ptr<Item> leftHand, rightHand;

  bool blink;

  void UpdateInput(RunningState &state);
  bool UseItem(RunningState &state, const std::shared_ptr<Item> &item);

  void QueueElement(Element element);
  void ClearElements();
  void CastSpell(RunningState &state);
};

#endif
