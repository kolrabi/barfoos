#ifndef BARFOOS_GUI_H
#define BARFOOS_GUI_H

#include "common.h"
#include "2d.h"
#include "icolor.h"

#include <unordered_map>

struct NinePatch {
  const Texture *texture;
  Rect srcRects[9];
  
  NinePatch() : texture(nullptr) {}
  NinePatch(const std::string &name);
  NinePatch(const std::string &name, const Rect &innerRect);
  std::vector<Vertex> GetVerts(const Rect &rect) const;
};

enum class GuiState : size_t {
  Normal   = 0,
  Disabled = 1,
  Hover    = 2,
  Active   = 3
};

namespace std { template<> struct hash<GuiState> {
  size_t operator()(const GuiState &type) const { return (size_t)type; }
}; }

class Gui {
public:

  Gui();
  virtual ~Gui();

  virtual void Update(Game &game);
  virtual void HandleEvent(const InputEvent &event);
  virtual void OnHide() {}
  virtual void OnShow() {}

  virtual void Draw(Gfx &gfx, const Point &parentPos);
  virtual void DrawTooltip(Gfx &gfx, const Point &parentPos);

  bool IsOver(const Point &p) const;
  
  void AddChild(Gui *child);
  Gui *GetChildAt(const Point &p);
 
  void SetSize(const Point &s);
  void SetPosition(const Point &p);
  void SetCenter(const Point &p);
  void SetGravity(bool gravN, bool gravE, bool gravS, bool gravW);

  void SetText(const std::string &str);
  
  std::function<void(Gui *)> GetOnActivate() { return onActivate; }
  void SetOnActivate(std::function<void(Gui *)> func) { onActivate = func; }
  
  void SetBackground(GuiState state, const NinePatch &patch) { this->backgrounds[state] = patch; }
  void SetColor(GuiState state, const IColor &color) { this->colors[state] = color; }

  void SetBackground(const NinePatch &patch) { 
    this->backgrounds[GuiState::Normal] = patch; 
    this->backgrounds[GuiState::Disabled] = patch; 
    this->backgrounds[GuiState::Hover] = patch; 
    this->backgrounds[GuiState::Active] = patch; 
  }
  void SetColor(const IColor &color) { 
    this->colors[GuiState::Normal] = color; 
    this->colors[GuiState::Disabled] = color; 
    this->colors[GuiState::Hover] = color; 
    this->colors[GuiState::Active] = color; 
  }
  
  void SetEnabled(bool enable);
  GuiState GetGuiState() const { return state; }

protected:

  Rect rect;
  Point lastVSize;

  std::vector<Gui*> children;

  bool gravN, gravE, gravS, gravW;
  bool updateGravity;
  
  RenderString *text;
  
  GuiState state;
  
  std::unordered_map<GuiState, NinePatch> backgrounds;
  std::unordered_map<GuiState, IColor> colors;
  
  std::function<void(Gui *)> onActivate;
};

#endif

