#ifndef BARFOOS_PROPERTIES_H
#define BARFOOS_PROPERTIES_H

#include "common.h"

struct IColor;

struct Properties {
  
  Properties() :
    game(nullptr),
    tokens(),
    lastError("")
  {}
  
  virtual ~Properties() {}

  void ParseFile(FILE *f);
  
  virtual void ParseProperty(const std::string &name) = 0;
  
  void Parse(int &i);
  void Parse(size_t &i);
  void Parse(float &f);
  void Parse(Vector3 &v);
  void Parse(IVector3 &iv);
  void Parse(IColor &c);
  void Parse(const std::string &prefix, const Texture *&t);
  void Parse(const std::string &prefix, std::vector<const Texture *> &t);
  void Parse(std::string &s);
  void Parse(Element &e);
  void ParseSideMask(size_t &i);

  Game *game;
  
  bool operator==(const Properties &that) const {
    return this == &that;
  }

  bool operator!=(const Properties &that) const {
    return this != &that;
  }
  
private:

  std::vector<std::string> tokens;
  std::string lastError;
 
protected:

  void SetError(const std::string &);
};

#endif

