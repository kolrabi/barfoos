#ifndef BARFOOS_UTIL_H
#define BARFOOS_UTIL_H

#include "common.h"
#include <cstdio>
#include <string>

#include <sys/time.h>
#include <random>

struct Token {
  size_t line;
  size_t col;
  std::string text;
};

std::vector<Token> Tokenize(const std::string &text);

FILE *openAsset(const std::string &name);
std::string loadAssetAsString(const std::string &name);
time_t getFileChangeTime(const std::string &name);

unsigned int loadTexture(const std::string &name, unsigned int tex = 0);
void updateTextures();
std::vector <std::string> findAssets(const std::string &type);

void drawUnitCube();
void drawBillboard(const Vector3 &pos, float w, float h, unsigned int tex, float u=0, float uw=1);  
void drawAABB(const AABB &aabb);
float Wave(float x, float z, float t, float a = 0.2, float o = 0.5);

class Random {
public:

  Random(const std::string &s) {
    std::seed_seq seq(s.begin(), s.end());
    gen = std::mt19937(seq);
  }
  
  uint32_t Integer(){
    return gen();
  }
  
  uint32_t Integer(uint32_t n) {
    return Integer() % n;
  }
  
  float Float01() {
    return (Integer() / (float)(0x100000000));
  }

  float Float() {
    return Float01()*2-1;
  }
  
  bool Coin() {
    return Integer()%2;
  }
  
  bool Chance(float p) {
    return Float() <= p;
  }
   
private:

  std::mt19937 gen;
};

#endif

