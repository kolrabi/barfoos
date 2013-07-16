#ifndef BARFOOS_SERIALIZER_H
#define BARFOOS_SERIALIZER_H

#include "common.h"

class Serializer {
public:

  Serializer(const char *magic, size_t magiclen=4);
  Serializer(const Serializer &) = delete;
  Serializer(Serializer &&) = delete;
  ~Serializer();
  
  Serializer &operator=(const Serializer &) = delete;

  Serializer &operator << (const uint8_t &);
  Serializer &operator << (const uint16_t &);
  Serializer &operator << (const uint32_t &);
  Serializer &operator << (const uint64_t &);

  Serializer &operator << (const int8_t &);
  Serializer &operator << (const int16_t &);
  Serializer &operator << (const int32_t &);
  Serializer &operator << (const int64_t &);

  Serializer &operator << (const float &);
  Serializer &operator << (const std::string &str);

  bool WriteToFile(const std::string &fileName);
  
private:

  std::vector<std::string> strings;
  size_t byteCount;
  size_t byteCapacity;
  uint8_t *bytes;
  
  char *magic;
  size_t magicLen;

  void GrowBy(size_t n);
  size_t AddString(const std::string &str);
};

#endif

