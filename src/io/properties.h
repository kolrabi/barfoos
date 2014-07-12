#ifndef BARFOOS_PROPERTIES_H
#define BARFOOS_PROPERTIES_H

#include <cstdio>

#include "math/vector3.h"
#include "math/ivector3.h"
#include "util/icolor.h"

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
  void Parse(uint32_t &i);
  void Parse(int16_t &i);
  void Parse(float &f);
  void Parse(Vector3 &v);
  void Parse(IVector3 &iv);
  void Parse(IColor &c);
  void Parse(const std::string &prefix, const Texture *&t);
  void Parse(const std::string &prefix, std::vector<const Texture *> &t);
  void Parse(std::string &s);
  void Parse(std::vector<std::string> &s);
  void Parse(Element &e);
  void Parse(SpawnClass &s);
  void ParseSideMask(uint32_t &i);

  template<class K, class V>
  void Parse(std::unordered_map<K,V> &map) {
    K key;
    V value;

    Parse(key);
    Parse(value);
    map[key] = value;
  }

  template<class V>
  void Parse(std::vector<V> &vect) {
    V value;

    Parse(value);
    vect.push_back(value);
  }

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

