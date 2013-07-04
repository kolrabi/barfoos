#ifndef BARFOOS_INVENTORY_GUI_H
#define BARFOOS_INVENTORY_GUI_H

#include "common.h"
#include "gui.h"

class Entity;
class InventoryGui;
class Item;
class Gfx;
struct InputEvent;

class InventorySlotGui : public Gui {
public: 
  InventorySlotGui(InventoryGui *parent, Entity &entity, InventorySlot slot);
  virtual ~InventorySlotGui();

  virtual void Draw(Gfx &gfx, const Point &parentPos) override;
  virtual void HandleEvent(const InputEvent &event) override;

private:

  InventoryGui *parent;
  Entity &entity;
  InventorySlot slot;
  bool hover;

  const Texture *slotTex;
};

class InventoryGui : public Gui {
public:
  
  InventoryGui(Game &game, Entity &entity);
  virtual ~InventoryGui();

  virtual void Update(Game &game) override;
  virtual void HandleEvent(const InputEvent &event) override;
  virtual void OnHide();

  virtual void Draw(Gfx &gfx, const Point &parentPos) override;  
  
  void SetForward(const Vector3 &forward) { this->forward = forward; }

  std::shared_ptr<Item> dragItem;

protected:

  Entity &entity;
  Point mousePos;
  Vector3 forward;
  
  bool dropItem;

private:

  Gui* AddSlotGui(const Point &p, InventorySlot slot);
};

#endif

