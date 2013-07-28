#ifndef BARFOOS_MARKOV_H
#define BARFOOS_MARKOV_H

#include "weighted_map.h"

template<class T>
class markov_chain : public std::unordered_map<T, weighted_map<T>> {
public:

  markov_chain() {}

  template<class I>
  markov_chain(T initial, I from, I to) {
    self.add(initial, from, to);
  }
  
  template<class I>
  void add(T initial, I from, I to) {
    if (to == from) return;
    
    T prev(initial);
    
    for (I i = from; i!=to; ++i) {
      self[prev][*i] ++;
      prev = *i;
    }
  }
  
};

#endif
