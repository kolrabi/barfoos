#ifndef BARFOOS_DESERIALIZER_H
#define BARFOOS_DESERIALIZER_H

#include <unordered_map>
#include <vector>
#include <list>

class Deserializer {
public:

  Deserializer();
  Deserializer(const Deserializer &) = delete;
  Deserializer(Deserializer &&) = delete;
  ~Deserializer();
  
  Deserializer &operator=(const Deserializer &) = delete;

  Deserializer &operator >> (uint8_t &);
  Deserializer &operator >> (uint16_t &);
  Deserializer &operator >> (uint32_t &);
  Deserializer &operator >> (uint64_t &);

  Deserializer &operator >> (int8_t &);
  Deserializer &operator >> (int16_t &);
  Deserializer &operator >> (int32_t &);
  Deserializer &operator >> (int64_t &);
  
  Deserializer &operator >> (float &);
  Deserializer &operator >> (bool &);
  Deserializer &operator >> (std::string &str);
  Deserializer &operator >> (IVector3 &v);
  Deserializer &operator >> (Vector3 &v);
  Deserializer &operator >> (IColor &v);

  Deserializer &operator >> (Element &v);
  
  template<class T>
  Deserializer &operator >> (std::vector<T> &v) {
    uint32_t size;
    self >> size;
    Log("vector: %u\n", size);
    for (size_t i = 0; i<size; i++) {
      T data;
      self >> data;
      v.push_back(data);
    }
    return *this;
  }

  template<class T>
  Deserializer &operator >> (std::list<T> &v) {
    uint32_t size;
    self >> size;
    for (size_t i = 0; i<size; i++) {
      T data;
      self >> data;
      v.push_back(data);
    }
    return *this;
  }
  
  template<class S, class T>
  Deserializer &operator >> (std::unordered_map<S, T> &v) {
    uint32_t size;
    self >> size;
    for (size_t i = 0; i<size; i++) {
      S s; 
      T t;
      self >> s;
      self >> t;
      v[s] = t;
    }
    return *this;
  }

  template<class S, class T>
  Deserializer &operator >> (std::unordered_map<S, T*> &v) {
    uint32_t size;
    self >> size;
    for (size_t i = 0; i<size; i++) {
      S s; 
      T *t;
      self >> s;
      self >> t;
      v[s] = t;
    }
    return *this;
  }
  
  Deserializer &operator >> (std::vector<bool> &v);

  bool LoadFromFile(FILE *f, const char *magic, size_t magiclen = 4);
  size_t GetPos() const { return bytePos; }
  
private:

  std::vector<std::string> strings;
  size_t byteCount, bytePos;
  uint8_t *bytes;
};

#endif

