#ifndef BARFOOS_INPUT_H
#define BARFOOS_INPUT_H

#include "common.h"

enum class InputKey {
  Invalid = 0,

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
  DebugDie,
  DebugNoclip
};

enum class InputEventType {
  Invalid = 0,
  ScreenResize,
  
  MouseMove,
  MouseDelta,
  
  Key
};

struct InputEvent {
  InputEventType type = InputEventType::Invalid;
  Point p;
  InputKey key;
  bool down = false;
  
  InputEvent(InputEventType type, const Point &p) : type(type), p(p), key(InputKey::Invalid) {}
  InputEvent(InputEventType type, const Point &p, InputKey key, bool down) : type(type), p(p), key(key), down(down) {}
};

class Input final {
public:

  Input();
  ~Input();

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
  
  void HandleEvent(const InputEvent &event);
  
  size_t AddHandler(std::function<void(const InputEvent &)> handler);
  void RemoveHandler(size_t id);
  
private:

  struct Handler {
    std::function<void(const InputEvent &)> handler;
    size_t id;
    
    Handler(std::function<void(const InputEvent &)> handler, size_t id) : handler(handler), id(id) {}
    
    bool operator==(const size_t &rhs) {
      return id == rhs;
    }
  };

  size_t nextHandlerId;
  std::vector<Handler> handlers;

  std::map<InputKey, bool> activeKeys;
  std::map<InputKey, bool> lastActiveKeys;

  void SetKey(int key, bool down);
};

#endif
