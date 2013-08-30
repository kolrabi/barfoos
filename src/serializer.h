#ifndef BARFOOS_SERIALIZER_H
#define BARFOOS_SERIALIZER_H

#include <unordered_map>
#include <vector>
#include <list>

class Serializer {
public:

  Serializer(const char *magic, size_t magiclen=4);
  Serializer(const Serializer &) = delete;
  Serializer(Serializer &&) = delete;
  ~Serializer();
  
  Serializer &operator=(const Serializer &) = delete;

  Serializer &operator << (uint8_t);
  Serializer &operator << (uint16_t);
  Serializer &operator << (uint32_t);
  Serializer &operator << (uint64_t);

  Serializer &operator << (int8_t);
  Serializer &operator << (int16_t);
  Serializer &operator << (int32_t);
  Serializer &operator << (int64_t);
  
  Serializer &operator << (float);
  Serializer &operator << (bool);
  Serializer &operator << (const std::string &str);
  Serializer &operator << (const IVector3 &v);
  Serializer &operator << (const Vector3 &v);
  Serializer &operator << (const IColor &v);
  
  template<class T>
  Serializer &operator << (const std::vector<T> &v) {
    self << (uint32_t)v.size();
    for (auto &e:v) self << e;
    return *this;
  }
  
  template<class T>
  Serializer &operator << (const std::list<T> &v) {
    self << (uint32_t)v.size();
    for (auto &e:v) self << e;
    return *this;
  }

  template<class S, class T>
  Serializer &operator << (const std::unordered_map<S, T> &v) {
    self << (uint32_t)v.size();
    for (auto &e:v) self << e.first << e.second;
    return *this;
  }
  
  Serializer &operator << (const std::vector<bool> &v);

  bool WriteToFile(FILE *file);
  
private:

  std::vector<std::string> strings;
  size_t byteCount;
  size_t byteCapacity;
  uint8_t *bytes;
  
  char *magic;
  size_t magicLen;

  void GrowBy(size_t n);
  uint32_t AddString(const std::string &str);
};

#endif

