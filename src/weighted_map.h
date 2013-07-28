#ifndef BARFOOS_WEIGHTED_MAP_H
#define BARFOOS_WEIGHTED_MAP_H

#include <unordered_map>

template<class T>
class weighted_map : public std::unordered_map<T, float> {

public:

  T select(float index) const {
    if (this->size() == 0) return T();
    if (this->size() == 1) return this->begin()->first;
    std::vector<weighted_object> totals;
    float total = 0;
    for (auto &entry : *this) {
      total += entry.second;
      
      weighted_object w;
      w.object = entry.first;
      w.weight = total;
      totals.push_back(w);
    }
    if (total == 0) return T();
    index *= total;
    
    for (weighted_object w : totals) {
      if (index < w.weight) {
        return w.object;
      }
    }
    return T();
  }

private:

  struct weighted_object {
    T object;
    float weight;
  };
};

#endif
