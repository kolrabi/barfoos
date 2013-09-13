#ifndef BARFOOS_TRIGGER_H
#define BARFOOS_TRIGGER_H

#include "common.h"

class Triggerable {
public:

  virtual void SetTrigger(uint32_t id, bool toggle = false) = 0;
  virtual uint32_t GetTriggerId() const = 0;
  virtual bool IsTriggerToggle() const = 0;

  virtual void SetTriggered(bool triggered) = 0;
  virtual bool IsTriggered() const = 0;
  
  void TriggerOn();
  void TriggerOff();
};

#endif

