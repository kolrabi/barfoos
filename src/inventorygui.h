#ifndef BARFOOS_INVENTORY_GUI_H
#define BARFOOS_INVENTORY_GUI_H

#include "common.h"
#include "gui.h"

class InventorySlotGui : public Gui {
public: 
  InventorySlotGui(InventoryGui *parent, Entity &entity, InventorySlot slot);
  InventorySlotGui(const InventorySlotGui &) = delete;
  virtual ~InventorySlotGui();

  InventorySlotGui &operator=(const InventorySlotGui &) = delete;
  
  virtual void Draw(Gfx &gfx, const Point &parentPos) override;
  virtual void HandleEvent(const InputEvent &event) override;

private:

  Entity &entity;
  InventoryGui *parent;
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

  std::shared_ptr<Item> dragItem;

protected:

  Entity &entity;
  Point mousePos;
  
  bool dropItem;

private:

  Gui* AddSlotGui(const Point &p, InventorySlot slot);
};

#endif

