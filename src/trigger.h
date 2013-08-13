#ifndef BARFOOS_TRIGGER_H
#define BARFOOS_TRIGGER_H

#include "common.h"

class Triggerable {
public:

  Triggerable();
  
  void SetTrigger(uint32_t id, bool toggle = false);
  uint32_t GetTriggerId() const;
  bool IsTriggered() const;
  
  void TriggerOn();
  void TriggerOff();
  
private:

  uint32_t triggerID;
  bool isToggle;
  bool triggered;
  
  friend Serializer &operator << (Serializer &ser, const Triggerable &trig);
  friend Deserializer &operator >> (Deserializer &deser, Triggerable &trig);
};

#endif

