#include "inventorygui.h"
#include "entity.h"
#include "util.h"
#include "item.h"
#include "itementity.h"
#include "gfx.h"
#include "input.h"
#include "texture.h"
#include "inventory.h"
#include "text.h"
#include "runningstate.h"

InventoryGui::InventoryGui(RunningState &state, Entity &entity) : 
  dragItem(nullptr),
  state(state),
  entity(entity), 
  mousePos(),
  dropItem(false),
  name("Equipment")
{
  //                 x  x  x  x
  //   8  0          x  x  x  x
  //   4  1  5       x  x  x  x
  //   6  2  7       x  x  x  x
  //      3          x  x  x  x
  //                 x  x  x  x

  Gfx &gfx = state.GetGame().GetGfx();
  Point vsize = gfx.GetVirtualScreenSize();

  Point slotDist(36, 36);
  Point topLeft  = gfx.AlignTopLeftScreen(Point(32,32), 16)  + Point(0, 16);
  Point topRight = gfx.AlignTopRightScreen(Point(32,32), 16) + Point(0, 16);

  Gui *bg = new Gui();
  bg->SetPosition(Point(8,24));
  bg->SetSize(Point(124, 172));
  bg->SetBackground(NinePatch("gui/button"));
  this->children.push_back(bg);
  
  bg = new Gui();
  bg->SetPosition(Point(vsize.x - (8+156),24));
  bg->SetSize(Point(156, 172));
  bg->SetBackground(NinePatch("gui/button"));
  bg->SetGravity(true, true, false, false);
  this->children.push_back(bg);

  AddSlotGui(entity, topLeft+Point(slotDist.x*0, slotDist.y*0), InventorySlot::Amulet);
  AddSlotGui(entity, topLeft+Point(slotDist.x*1, slotDist.y*0), InventorySlot::Helmet);
  AddSlotGui(entity, topLeft+Point(slotDist.x*0, slotDist.y*1), InventorySlot::LeftRing);
  AddSlotGui(entity, topLeft+Point(slotDist.x*1, slotDist.y*1), InventorySlot::Armor);
  AddSlotGui(entity, topLeft+Point(slotDist.x*2, slotDist.y*1), InventorySlot::RightRing);
  AddSlotGui(entity, topLeft+Point(slotDist.x*0, slotDist.y*2), InventorySlot::LeftHand);
  AddSlotGui(entity, topLeft+Point(slotDist.x*1, slotDist.y*2), InventorySlot::Greaves);
  AddSlotGui(entity, topLeft+Point(slotDist.x*2, slotDist.y*2), InventorySlot::RightHand);
  AddSlotGui(entity, topLeft+Point(slotDist.x*1, slotDist.y*3), InventorySlot::Boots);

  InventorySlot slot = InventorySlot::Backpack0;
  while (slot < InventorySlot::End) {
    size_t index = (size_t)slot;
    int x = index%4 -3;
    int y = (index - (size_t)InventorySlot::Backpack0)/4;
    AddSlotGui(entity, topRight+Point(slotDist.x*x, slotDist.y*y), slot)->SetGravity(true, true, false, false);
    
    slot = InventorySlot(index + 1);
  }
  
  this->dragItem = nullptr;
  this->dropItem = false;
}

InventoryGui::InventoryGui(RunningState &state, Entity &entity, Entity &other) : 
  dragItem(nullptr),
  state(state),
  entity(entity), 
  mousePos(),
  dropItem(false),
  name(other.GetName())
{
  Gfx &gfx = state.GetGame().GetGfx();
  Point vsize = gfx.GetVirtualScreenSize();

  Point slotDist(36, 36);
  Point topLeft  = gfx.AlignTopLeftScreen(Point(32,32), 16)  + Point(0, 16);
  Point topRight = gfx.AlignTopRightScreen(Point(32,32), 16) + Point(0, 16);

  Gui *bg = new Gui();
  bg->SetPosition(Point(8,24));
  bg->SetSize(Point(156, 172));
  bg->SetBackground(NinePatch("gui/button"));
  this->children.push_back(bg);
  
  bg = new Gui();
  bg->SetPosition(Point(vsize.x - (8+156),24));
  bg->SetSize(Point(156, 172));
  bg->SetBackground(NinePatch("gui/button"));
  bg->SetGravity(true, true, false, false);
  this->children.push_back(bg);
  
  InventorySlot slot = InventorySlot::Backpack0;
  while (slot < InventorySlot::End) {
    size_t index = (size_t)slot;
    int x = index%4;
    int y = (index - (size_t)InventorySlot::Backpack0)/4;
    AddSlotGui(other, topLeft+Point(slotDist.x*x, slotDist.y*y), slot)->SetGravity(false, false, true, true);
    
    slot = InventorySlot(index + 1);
  }

  slot = InventorySlot::Backpack0;
  while (slot < InventorySlot::End) {
    size_t index = (size_t)slot;
    int x = index%4 -3;
    int y = (index - (size_t)InventorySlot::Backpack0)/4;
    AddSlotGui(entity, topRight+Point(slotDist.x*x, slotDist.y*y), slot)->SetGravity(true, true, false, false);
    
    slot = InventorySlot(index + 1);
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
    entity.GetInventory().DropItem(this->dragItem);
    this->dragItem = nullptr;
  }
  this->dropItem = false;
}

void InventoryGui::Draw(Gfx &gfx, const Point &parentPos) {
  Gui::Draw(gfx, parentPos);
  
  if (dragItem != nullptr) {
    dragItem->DrawIcon(gfx, mousePos);
  }

  Point vsize = gfx.GetVirtualScreenSize();

  RenderString(this->name).Draw(gfx, Point(16,180));
  RenderString(this->entity.GetName()).Draw(gfx, Point(vsize.x-16,180), int(Align::HorizRight));
}

void InventoryGui::HandleEvent(const InputEvent &event) {
  Gui::HandleEvent(event);
  
  if (event.type == InputEventType::MouseMove) {
    mousePos = event.p;
  } else if (event.type == InputEventType::Key && event.key == InputKey::MouseLeft) {
    if (!event.down && this->dragItem) {
      this->dropItem = true;
    }
  }
}

Gui *InventoryGui::AddSlotGui(Entity &entity, const Point &p, InventorySlot slot) {
  Gui *gui = new InventorySlotGui(this, entity, slot);
  gui->SetPosition(p - Point(16,16));
  gui->SetSize(Point(32,32));
  this->children.push_back(gui);
  return gui;
}

void InventoryGui::OnHide() {
  if (dragItem) {
    Log("InventoryGui::OnHide putting back %s %u\n", dragItem->GetDisplayName().c_str(), dragItem->GetAmount());
    entity.GetInventory().AddToBackpack(dragItem);
    this->dragItem = nullptr;
  }
}

// -----------------------------------------------------------------------

InventorySlotGui::InventorySlotGui(
  InventoryGui *parent, Entity &entity, InventorySlot slot
) : 
  entity(entity),
  parent(parent),
  slot(slot),
  hover(false),
  slotTex(loadTexture("gui/slot"))
{}

InventorySlotGui::~InventorySlotGui() {
}
  
void InventorySlotGui::HandleEvent(const InputEvent &event) {
  this->hover = this->IsOver(event.p);

  if (event.type != InputEventType::Key) return;
  if (!this->IsOver(event.p)) return;

  Inventory &inv(entity.GetInventory());
  
  if (event.key == InputKey::MouseLeft) {
    
    if (event.down) {
    
      std::shared_ptr<Item> item(inv[slot]);
      if (item && (!item->IsCursed() || !item->IsEquipped())) {
        parent->dragItem = item;
        inv.Equip(nullptr, slot);
      }
      
    } else {
    
      std::shared_ptr<Item> item(parent->dragItem);
      if (!item) return;
      Log("InventoryGui::HandleEvent putting item %s %u in slot %u\n", item->GetDisplayName().c_str(), item->GetAmount(), slot);
      inv.AddToInventory(item, slot);
      parent->dragItem = nullptr;
    }
    
  } else if (event.key == InputKey::MouseRight) {
    if (event.down) {
      if (parent->dragItem) {
        // combine one of the dragged items with the one under cursor
        std::shared_ptr<Item> combineItem = parent->dragItem;
        bool isStack = parent->dragItem->GetAmount() > 1;
        if (isStack) {
          combineItem = std::shared_ptr<Item>(new Item(parent->dragItem->GetProperties().name));
          parent->dragItem->DecAmount();
        } else {
          parent->dragItem = nullptr;
        }
        
        inv.AddToInventory(combineItem, slot);
      } else {
      
        std::shared_ptr<Item> item(inv[slot]);
        if (item && item->IsConsumable()) {
          inv.ConsumeItem(slot, parent->GetEntity());
        }
      }
    } 
  }
}

void 
InventorySlotGui::Draw(Gfx &gfx, const Point &parentPos) {
  Gui::Draw(gfx, parentPos);
  
  const Texture *tex = slotTex;
  Sprite sprite;
  sprite.texture = tex;

  Point p = rect.pos+parentPos+Point(16,16);
  if (this->hover) {
    p = p - Point(2,2);
  }
  
  gfx.DrawIcon(sprite, p);

  std::shared_ptr<Item> item = entity.GetInventory()[slot];
  
  if (item != nullptr) {
    item->DrawIcon(gfx, p);
  }
}

void 
InventorySlotGui::DrawTooltip(Gfx &gfx, const Point &parentPos) {
  Gui::DrawTooltip(gfx, parentPos);
  
  std::shared_ptr<Item> item = entity.GetInventory()[slot];
  Point pos(rect.pos+parentPos);
  Point vscreen = gfx.GetVirtualScreenSize();
  
  if (this->hover && item) {
    RenderString name(item->GetDisplayName());
    RenderString stat(item->GetDisplayStats().GetToolTip(), "small");
    
    if (pos.x < vscreen.x / 2) pos.x += 32; else pos.x -= std::max(name.GetSize().x, stat.GetSize().x);
  
    name.Draw(gfx, pos);
    stat.Draw(gfx, pos + Point(0, 8));
  }
}
