#include "properties.h"

#include "icolor.h"
#include "texture.h"
#include "vector3.h"
#include "ivector3.h"

std::vector<std::string> Tokenize(const char *l) {
  std::vector<std::string> tokens;
  
  std::string line = l;
  char *p = &line[0];
  char *q;
  
  // skip whitespace
  while(*p && strchr(" \r\n\t", *p)) p++;
  
  // end of string or first character is #?
  if (*p == 0 || *p == '#') return tokens;
  
  do {
    // find end of token
    q = p;
    while(*q && !strchr(" \r\n\t", *q)) q++;
    
    // terminate token
    if (q) *q = 0;
    tokens.push_back(p);
    
    // not end of line? then skip whitespace to next token
    if (q) { 
      p = q+1; 
      while(*p && strchr(" \r\n\t", *p)) p++;
    }
  } while(q && *p);
  return tokens;
}

void 
Properties::ParseFile(FILE *f) {
  char line[256];
  int l = 0;
  
  while(fgets(line, 256, f) && !feof(f)) {
    l++;
    tokens = Tokenize(line);
    if (tokens.size() == 0) continue;
    
    std::string cmd = tokens[0];
    for (auto &c:cmd) c = ::tolower(c);

    tokens.erase(tokens.begin());
    
    ParseProperty(cmd);
    
    if (lastError != "") {
      std::cerr << "Line " << l << ": " << lastError << std::endl;
      lastError = "";
    }
  }    
}
  
void 
Properties::Parse(int &i) {
  if (tokens.size() == 0) {
    this->SetError("unexpected end of line, expecting int");
    return;
  }
  i = std::atoi(tokens[0].c_str());
  tokens.erase(tokens.begin());
}

void 
Properties::Parse(size_t &i) {
  if (tokens.size() == 0) {
    this->SetError("unexpected end of line, expecting uint");
    return;
  }
  i = std::atoi(tokens[0].c_str());
  tokens.erase(tokens.begin());
}

void 
Properties::Parse(float &f) {
  if (tokens.size() == 0) {
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
  if (tokens.size() == 0) {
    this->SetError("unexpected end of line, expecting texture name");
    return;
  }
  t = loadTexture(prefix+tokens[0]);
  tokens.erase(tokens.begin());
}

void 
Properties::Parse(std::string &str) {
  if (tokens.size() == 0) {
    this->SetError("unexpected end of line, expecting string");
    return;
  }
  str = tokens[0];
  tokens.erase(tokens.begin());
}

void 
Properties::SetError(const std::string &error) {
  if (this->lastError != "") return;
  this->lastError = error;
}
