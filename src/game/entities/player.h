#ifndef BARFOOS_PLAYER_H
#define BARFOOS_PLAYER_H

#include "mob.h"

#include <unordered_map>
#include <list>

class Player final : public Mob {
public:

  Player();
  Player(const Entity_Proto &proto);
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

  void                      SetUniforms(const std::shared_ptr<Shader> &shader) const;

  virtual const Entity_Proto &GetProto() override;

  void                      SetBobPhase(float f)                    { this->proto.mutable_player()->set_bob_phase(f); }
  float                     GetBobPhase()                     const { return this->proto.player().bob_phase(); }

  void                      SetBobAmplitude(float f)                { this->proto.mutable_player()->set_bob_amplitude(f); }
  float                     GetBobAmplitude()                 const { return this->proto.player().bob_amplitude(); }

  void                      SetMessageY(float f)                    { this->proto.mutable_player()->set_message_y(f); }
  float                     GetMessageY()                     const { return this->proto.player().message_y(); }

  void                      SetMessageVY(float f)                   { this->proto.mutable_player()->set_message_vy(f); }
  float                     GetMessageVY()                    const { return this->proto.player().message_vy(); }

  void                      SetPain(float f)                        { this->proto.mutable_player()->set_pain(f); }
  float                     GetPain()                         const { return this->proto.player().pain(); }

  void                      SetHPFlashTime(float f)                 { this->proto.mutable_player()->set_hp_flash_time(f); }
  float                     GetHPFlashTime()                  const { return this->proto.player().hp_flash_time(); }

  void                      SetYaw(float f)                         { this->proto.mutable_player()->set_yaw(f); }
  float                     GetYaw()                          const { return this->proto.player().yaw(); }

  void                      SetPitch(float f)                       { this->proto.mutable_player()->set_pitch(f); }
  float                     GetPitch()                        const { return this->proto.player().pitch(); }

  void                      SetCastStartTime(float f)               { this->proto.mutable_player()->set_cast_start_time(f); }
  float                     GetCastStartTime()                const { return this->proto.player().cast_start_time(); }

  void                      SetLastCastTime(float f)                { this->proto.mutable_player()->set_last_cast_time(f); }
  float                     GetLastCastTime()                 const { return this->proto.player().last_cast_time(); }

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

  // rendering
  const Texture *crosshairTex;
  const Texture *slotTex;

  std::unordered_map<Element, Sprite> gemSprites;
  Entity *lookAtEntity = nullptr;

  // gameplay
  bool itemActiveLeft, itemActiveRight;
  bool lastItemActiveLeft, lastItemActiveRight;

  std::vector<Element> elements;

  // display
  std::list<Message*> messages;
  float fps;

  // TODO: add to proto
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
  void StopCasting();
};

#endif
