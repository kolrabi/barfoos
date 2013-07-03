#include <GLFW/glfw3.h>

#include "input.h"

Input *Input::Instance = nullptr;

Input::Input() {
  delete Input::Instance;
  Input::Instance = this;
}

Input::~Input() {
  Input::Instance = nullptr;
}

void 
Input::Update() {
  lastActiveKeys = activeKeys;
}

void 
Input::HandleKeyEvent(
  int k,
  bool down
) {  
  InputKey key;
  
  switch(k) {
    case 'W': key = InputKey::Forward; break;
    case 'S': key = InputKey::Backward; break;
    case 'A': key = InputKey::Left; break;
    case 'D': key = InputKey::Right; break;
    case ' ': key = InputKey::Jump; break;
    case GLFW_KEY_LEFT_SHIFT: key = InputKey::Sneak; break;
    
    case GLFW_KEY_F1: key = InputKey::DebugDie; break;
    case GLFW_KEY_F2: key = InputKey::DebugEntityAABB; break;
    case GLFW_KEY_F3: key = InputKey::DebugWireframe; break;
    default : return;
  }
  activeKeys[key] = down;
  
  /*
  AttackLeft,
  AttackRight,
  Use,
  
  Inventory,
  
  Escape,
  
  DebugWireframe,
  DebugEntityAABB
  */
}
