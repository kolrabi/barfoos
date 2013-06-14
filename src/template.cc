#include "template.h"
#include "templates/list.h"

static std::map<std::string, Template *> allTemplates;

const Template *
GetTemplate(
  const std::string &name
) {
  auto iter = allTemplates.find(name);
  if (iter == allTemplates.end()) {
    return nullptr;
    std::cerr << "template " << name << ": not found" << std::endl;
  }    
  return iter->second;
}

void
LoadTemplates() {
  allTemplates["floor_stone"] = new FloorTemplate("rock", 0, 0);
  allTemplates["floor_rock"] = new FloorTemplate("rock", 0.2, 0.2);
  allTemplates["floor_dirt"] = new FloorTemplate("dirt", 0.2);
}

