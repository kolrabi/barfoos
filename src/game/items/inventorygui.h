#ifndef BARFOOS_INVENTORY_GUI_H
#define BARFOOS_INVENTORY_GUI_H

#include "common.h"
#include "gui/gui.h"

class InventorySlotGui : public Gui {
public: 
  InventorySlotGui(InventoryGui *parent, Entity &entity, InventorySlot slot);
  InventorySlotGui(const InventorySlotGui &) = delete;
  virtual ~InventorySlotGui();

  InventorySlotGui &operator=(const InventorySlotGui &) = delete;
  
  virtual void Update(Game &game) override;
  virtual void Draw(Gfx &gfx, const Point &parentPos) override;
  virtual void DrawTooltip(Gfx &gfx, const Point &parentPos) override;
  virtual void HandleEvent(const InputEvent &event) override;

private:

  Entity &entity;
  InventoryGui *parent;
  InventorySlot slot;
  bool hover;

  const Texture *slotTex;

  float lastT;
  float lastClickT;
};

class InventoryGui : public Gui {
public:
  
  InventoryGui(RunningState &state, Entity &entity);
  InventoryGui(RunningState &state, Entity &entity, Entity &other);
  virtual ~InventoryGui();

  virtual void Update(Game &game) override;
  virtual void HandleEvent(const InputEvent &event) override;
  virtual void OnHide();

  virtual void Draw(Gfx &gfx, const Point &parentPos) override;  

  Entity &GetEntity() { return entity; }

  std::shared_ptr<Item> dragItem;

protected:

  RunningState &state;
  Entity &entity;
  Point mousePos;
  
  bool dropItem;
  std::string name;

private:

  Gui* AddSlotGui(Entity &entity, const Point &p, InventorySlot slot);
};

#endif

