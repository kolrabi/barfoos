#include <algorithm>

#include "input.h"

Input::Input() :
  nextHandlerId(0),
  handlers(),
  activeKeys(),
  lastActiveKeys()
{}

Input::~Input() {
}

void 
Input::Update() {
  lastActiveKeys = activeKeys;
}

void 
Input::HandleEvent(const InputEvent &event) {
  if (event.type == InputEventType::Key) {
    this->activeKeys[event.key] = event.down;
  }
  
  for (auto &fn : this->handlers) {
    fn.handler(event);
  }
}
  
size_t 
Input::AddHandler(std::function<void(const InputEvent &)> handler) {
  this->handlers.push_back(Handler(handler, nextHandlerId));
  
  return nextHandlerId++;
}
  
void 
Input::RemoveHandler(size_t id) {
  Log("Removing %u\n", id);
  auto iter = std::find(this->handlers.begin(), this->handlers.end(), id);
  //Log("%p\n", id);
  if (iter != this->handlers.end())
    this->handlers.erase(iter);
}
