#ifndef BARFOOS_RANDOM_H
#define BARFOOS_RANDOM_H

#include <random>

#include "common.h"

class Random {
public:

  Random(const std::string &s, size_t n = 0) {
    Seed(s, n);
  }
  
  void Seed(const std::string &s, size_t n = 0) {
    std::seed_seq seq(s.begin(), s.end());
    gen = std::mt19937(seq);
    gen.discard(n+1);
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
