#ifndef BARFOOS_UTIL_H
#define BARFOOS_UTIL_H

#include <cstdint>
#include <vector>

// TODO: separate file? --------------------------------------------------------------

float Wave(float x, float z, float t, float a = 0.2);

std::vector<std::string> Tokenize(const char *line);
uint32_t ParseSidesMask(const std::string &str);

// TODO: separate file: regular.h ----------------------------------------------------

#include <functional>

class Regular {
public:
  Regular() : 
    interval(0.0),
    func(),
    t(0.0)
  {}

  Regular(float interval, std::function<void()> func) :
    interval(interval),
    func(func),
    t(0.0)
  {}
  
  void Update(float deltaT) {
    t += deltaT;
    while(t > interval) {
      t -= interval;
      func();
    }
  }

private:

  float interval;
  std::function<void()> func;
  float t;
};

#endif

