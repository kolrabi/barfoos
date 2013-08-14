#include "properties.h"

#include "icolor.h"
#include "texture.h"
#include "vector3.h"
#include "ivector3.h"
#include "util.h"
#include "stats.h"
#include "entity.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>

void 
Properties::ParseFile(FILE *f) {
  char line[256];
  int l = 0;
  
  while(fgets(line, 256, f) && !feof(f)) {
    l++;
    tokens = Tokenize(line);
    if (tokens.empty()) continue;
    
    std::string cmd = tokens[0];
    for (auto &c:cmd) c = ::tolower(c);

    tokens.erase(tokens.begin());
    
    ParseProperty(cmd);
    
    if (lastError != "") {
      Log("Line %d: %s\n", l, lastError.c_str());;
      lastError = "";
    }
  }    
}
  
void 
Properties::Parse(int &i) {
  if (tokens.empty()) {
    this->SetError("unexpected end of line, expecting int");
    return;
  }
  i = std::atoi(tokens[0].c_str());
  tokens.erase(tokens.begin());
}

void 
Properties::Parse(uint32_t &i) {
  if (tokens.empty()) {
    this->SetError("unexpected end of line, expecting uint");
    return;
  }
  i = std::atoi(tokens[0].c_str());
  tokens.erase(tokens.begin());
}

void 
Properties::Parse(float &f) {
  if (tokens.empty()) {
    this->SetError("unexpected end of line, expecting float");
    return;
  }
  f = std::atof(tokens[0].c_str());
  tokens.erase(tokens.begin());
}

void 
Properties::Parse(Vector3 &v) {
  Parse(v.x);
  Parse(v.y);
  Parse(v.z);
}

void 
Properties::Parse(IVector3 &v) {
  Parse(v.x);
  Parse(v.y);
  Parse(v.z);
}

void 
Properties::Parse(IColor &c) {
  int rgb[3];
  Parse(rgb[0]);
  Parse(rgb[1]);
  Parse(rgb[2]);
  c.r = rgb[0];
  c.g = rgb[1];
  c.b = rgb[2];
}

void 
Properties::Parse(const std::string &prefix, const Texture *&t) {
  if (tokens.empty()) {
    this->SetError("unexpected end of line, expecting texture name");
    return;
  }
  t = loadTexture(prefix+tokens[0]);
  tokens.erase(tokens.begin());
}

void 
Properties::Parse(const std::string &prefix, std::vector<const Texture *> &t) {
  const Texture *tex = nullptr;
  Parse(prefix, tex);
  if (tex) t.push_back(tex);
}


void 
Properties::Parse(std::string &str) {
  if (tokens.empty()) {
    this->SetError("unexpected end of line, expecting string");
    return;
  }
  str = tokens[0];
  tokens.erase(tokens.begin());
}

void 
Properties::Parse(Element &e) {
  std::string elementString = "physical";
  Parse(elementString);
  
  for (auto &c:elementString) c = ::tolower(c);
  
  if (elementString == "physical") {
    e = Element::Physical;
  } else if (elementString == "fire") {
    e = Element::Fire;
  } else if (elementString == "water") {
    e = Element::Water;
  } else if (elementString == "earth") {
    e = Element::Earth;
  } else if (elementString == "air") {
    e = Element::Air;
  } else if (elementString == "life") {  
    e = Element::Life;
  } else {
    SetError("Unknown element name: "+elementString);
  }
}

void 
Properties::Parse(SpawnClass &s) {
  std::string classString = "entity";
  Parse(classString);
  
  for (auto &c:classString) c = ::tolower(c);
  
  if (classString == "entity") {
    s = SpawnClass::EntityClass;
  } else if (classString == "mob") {
    s = SpawnClass::MobClass;
  } else if (classString == "item") {
    s = SpawnClass::ItemEntityClass;
  } else if (classString == "player") {
    s = SpawnClass::PlayerClass;
  } else if (classString == "projectile") {
    s = SpawnClass::ProjectileClass;
  } else {
    SetError("Unknown spawn class name: "+classString);
  }
}

void 
Properties::SetError(const std::string &error) {
  if (this->lastError != "") return;
  this->lastError = error;
}

void
Properties::ParseSideMask(uint32_t &sides) {
  if (tokens.empty()) {
    this->SetError("unexpected end of line, expecting string");
    return;
  }
  const std::string &str = tokens[0];

  sides = 0;
  for (auto c:str) {
    c = ::tolower(c);
    switch(c) {
      case 'l': sides |= 1<<Side::Left;     break;
      case 'r': sides |= 1<<Side::Right;    break;
      case 'u': sides |= 1<<Side::Up;       break;
      case 'd': sides |= 1<<Side::Down;     break;
      case 'f': sides |= 1<<Side::Forward;  break;
      case 'b': sides |= 1<<Side::Backward; break;
      default: break;
    }
  }
  tokens.erase(tokens.begin());
}

