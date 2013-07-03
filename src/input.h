#ifndef BARFOOS_INPUT_H
#define BARFOOS_INPUT_H

#include "common.h"

enum class InputKey {
  Forward,
  Backward,
  Left,
  Right,
  Jump,
  Sneak,
  
  MouseLeft,
  MouseRight,
  Use,
  
  Inventory,
  
  Escape,
  
  DebugWireframe,
  DebugEntityAABB,
  DebugDie
};

class Input final {
public:

  Input();
  ~Input();
  static Input *Instance;

  void Update();
  
  bool IsKeyActive(InputKey key) {
    return activeKeys[key];
  }
  
  bool IsKeyDown(InputKey key) {
    return activeKeys[key] && !lastActiveKeys[key];
  }
  
  bool IsKeyUp(InputKey key) {
    return !activeKeys[key] && lastActiveKeys[key];
  }
  
  void HandleKeyEvent(int key, bool down);
  
private:

  std::map<InputKey, bool> activeKeys;
  std::map<InputKey, bool> lastActiveKeys;
};

#endif
