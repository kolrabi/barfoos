#include "trigger.h"

#include "serializer.h"
#include "deserializer.h"

Triggerable::Triggerable() :
  triggerID(0),
  isToggle(false),
  triggered(false)
{
}

void Triggerable::SetTrigger(uint32_t id, bool toggle) {
  this->triggerID = id;
  this->isToggle = toggle;
}

uint32_t Triggerable::GetTriggerId() const {
  return this->triggerID;
}

bool Triggerable::IsTriggered() const {
  return triggered;
}

void Triggerable::TriggerOn() {
  if (isToggle) {
    triggered = !triggered;
  } else {
    triggered = true;
  }
}

void Triggerable::TriggerOff() {
  if (!isToggle) {
    triggered = false;
  }
}
