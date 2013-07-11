#include "inventorygui.h"
#include "entity.h"
#include "util.h"
#include "item.h"
#include "itementity.h"
#include "game.h"
#include "gfx.h"
#include "icolor.h"
#include "input.h"
#include "texture.h"

InventoryGui::InventoryGui(Game &game, Entity &entity) 
: entity(entity) {

  //                 x  x  x  x
  //   8  0          x  x  x  x
  //   4  1  5       x  x  x  x
  //   6  2  7       x  x  x  x
  //      3          x  x  x  x
  //                 x  x  x  x

  Gfx &gfx = *game.GetGfx();
  Point slotDist(36, 36);
  Point topLeft  = gfx.AlignTopLeftScreen(Point(32,32), 16);
  Point topRight = gfx.AlignTopRightScreen(Point(32,32), 16);

  AddSlotGui(topLeft+Point(slotDist.x*0, slotDist.y*0), InventorySlot::Amulet);
  AddSlotGui(topLeft+Point(slotDist.x*1, slotDist.y*0), InventorySlot::Helmet);
  AddSlotGui(topLeft+Point(slotDist.x*0, slotDist.y*1), InventorySlot::LeftRing);
  AddSlotGui(topLeft+Point(slotDist.x*1, slotDist.y*1), InventorySlot::Armor);
  AddSlotGui(topLeft+Point(slotDist.x*2, slotDist.y*1), InventorySlot::RightRing);
  AddSlotGui(topLeft+Point(slotDist.x*0, slotDist.y*2), InventorySlot::LeftHand);
  AddSlotGui(topLeft+Point(slotDist.x*1, slotDist.y*2), InventorySlot::Greaves);
  AddSlotGui(topLeft+Point(slotDist.x*2, slotDist.y*2), InventorySlot::RightHand);
  AddSlotGui(topLeft+Point(slotDist.x*1, slotDist.y*3), InventorySlot::Boots);

  for (size_t i=(size_t)InventorySlot::Backpack; i<entity.GetInventorySize(); i++) {
    int x = i%4-3;
    int y = (i-(size_t)InventorySlot::Backpack)/4;
    AddSlotGui(topRight+Point(slotDist.x*x, slotDist.y*y), (InventorySlot)i)->SetGravity(true, true, false, false);
  }
  
  this->dragItem = nullptr;
  this->dropItem = false;
}

InventoryGui::~InventoryGui() {
}

void InventoryGui::Update(Game &game) {
  Gui::Update(game);

  if (this->dropItem && this->dragItem) {
    // drop item into world
    Mob *itemEntity = new ItemEntity(dragItem);
  
    itemEntity->SetPosition(entity.GetAABB().center + this->forward);
    itemEntity->AddVelocity((this->forward + Vector3(0,1,0))*10);
    game.AddEntity(itemEntity);
    
    this->dragItem = nullptr;
  }
  this->dropItem = false;
}

void InventoryGui::Draw(Gfx &gfx, const Point &parentPos) {
  Gui::Draw(gfx, parentPos);
  
  if (dragItem != nullptr) {
    dragItem->DrawIcon(gfx, mousePos);
  }
}

void InventoryGui::HandleEvent(const InputEvent &event) {
  Gui::HandleEvent(event);
  
  if (event.type == InputEventType::MouseMove) {
    mousePos = event.p;
  } else if (event.type == InputEventType::Key && event.key == InputKey::MouseLeft) {
    if (event.down == false && this->dragItem) {
      this->dropItem = true;
    }
  }
}

Gui *InventoryGui::AddSlotGui(const Point &p, InventorySlot slot) {
  Gui *gui = new InventorySlotGui(this, entity, slot);
  gui->SetPosition(p - Point(16,16));
  gui->SetSize(Point(32,32));
  this->children.push_back(gui);
  return gui;
}

void InventoryGui::OnHide() {
  if (dragItem) {
    entity.AddToInventory(dragItem);
    this->dragItem = nullptr;
  }
}

// -----------------------------------------------------------------------

InventorySlotGui::InventorySlotGui(
  InventoryGui *parent, Entity &entity, InventorySlot slot
) : entity(entity) {
  this->parent = parent;
  this->slot = slot;
  this->slotTex = loadTexture("gui/slot");
  this->hover = false;
}

InventorySlotGui::~InventorySlotGui() {
}
  
void InventorySlotGui::HandleEvent(const InputEvent &event) {
  this->hover = this->IsOver(event.p);

  if (event.type != InputEventType::Key || event.key != InputKey::MouseLeft) return;
  if (!this->IsOver(event.p)) return;
  
  if (event.down) {
    std::shared_ptr<Item> item(entity.GetInventory(slot));
    if (item) {
      parent->dragItem = item;
      entity.Equip(nullptr, slot);
    }
  } else {
    std::shared_ptr<Item> item(parent->dragItem);
    if (!item) return;
    if (!entity.AddToInventory(item, slot)) {
      entity.AddToInventory(item);
    }
    parent->dragItem = nullptr;
  }
}

void 
InventorySlotGui::Draw(Gfx &gfx, const Point &parentPos) {
  const Texture *tex = slotTex;
  Sprite sprite;
  sprite.texture = tex;

  Point p = rect.pos+parentPos+Point(16,16);
  if (this->hover) p = p - Point(2,2);
  
  gfx.DrawIcon(sprite, p);

  std::shared_ptr<Item> item = entity.GetInventory(slot);
  
  if (item != nullptr)
    item->DrawIcon(gfx, p);
}

