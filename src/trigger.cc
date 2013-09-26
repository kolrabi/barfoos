#include "trigger.h"

void Triggerable::TriggerOn() {
  if (this->IsTriggerToggle()) {
    this->SetTriggered(!this->IsTriggered());
  } else {
    this->SetTriggered(true);
  }
}

void Triggerable::TriggerOff() {
  if (!this->IsTriggerToggle()) {
    this->SetTriggered(false);
  }
}
