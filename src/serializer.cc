#include <cstring>

#include "serializer.h"

Serializer::Serializer(
  const char *magic, 
  size_t magiclen
) {
  this->magicLen = magiclen;
  this->magic = new char[magiclen];
  memcpy(this->magic, magic, magiclen);
  
  this->byteCount = 0;
  this->byteCapacity = 1024;
  this->bytes = new uint8_t[this->byteCapacity];
}

Serializer::~Serializer() {
  delete [] this->magic;
  delete [] this->bytes;
}

void Serializer::GrowBy(size_t n) {
  if (n+this->byteCount < this->byteCapacity) 
    return;
    
  size_t newCapacity = this->byteCapacity;
  while (n+this->byteCount >= newCapacity) 
    newCapacity *= 2;
  
  uint8_t *newBytes = new uint8_t[newCapacity];
  memcpy(newBytes, this->bytes, this->byteCount);
  delete [] this->bytes;
  this->bytes = newBytes;
  this->byteCapacity = newCapacity;
}

size_t Serializer::AddString(const std::string &str) {
  auto iter = std::find(strings.begin(), strings.end(), str);
  if (iter == strings.end()) {
    strings.push_back(str);
    return strings.size()-1;
  }
  return iter - strings.begin();
}

bool Serializer::WriteToFile(const std::string &fileName) {
  FILE *f = fopen(fileName.c_str(), "wb");
  if (!f) return false;
  
  // TODO: write magic and strings
  
  if (fwrite(this->bytes, this->byteCount, 1, f) != 1) {
    fclose(f);
    return false;
  }
  
  fclose(f);
  return true;
}

Serializer &Serializer::operator << (const uint8_t &v) {
  GrowBy(1);
  this->bytes[this->byteCount++] = v;
  return *this;
}

Serializer &Serializer::operator << (const uint16_t &v) {
  GrowBy(2);
  (*this) << (const uint8_t)((v   )&0xFF);
  (*this) << (const uint8_t)((v>>8)&0xFF);
  return *this;
}

Serializer &Serializer::operator << (const uint32_t &v) {
  GrowBy(4);
  (*this) << (const uint16_t)((v    )&0xFFFF);
  (*this) << (const uint16_t)((v>>16)&0xFFFF);
  return *this;
}

Serializer &Serializer::operator << (const uint64_t &v) {
  GrowBy(8);
  (*this) << (const uint32_t)((v    )&0xFFFFFFFF);
  (*this) << (const uint32_t)((v>>32)&0xFFFFFFFF);
  return *this;
}

Serializer &Serializer::operator << (const int8_t &v) {
  return (*this) << (const uint8_t)v;
}

Serializer &Serializer::operator << (const int16_t &v) {
  return (*this) << (const uint16_t)v;
}

Serializer &Serializer::operator << (const int32_t &v) {
  return (*this) << (const uint32_t)v;
}

Serializer &Serializer::operator << (const int64_t &v) {
  return (*this) << (const uint64_t)v;
}

Serializer &Serializer::operator << (const float &v) {
  union {
    float f;
    uint32_t i;
  } u;
  u.f = v;
  return (*this) << u.i;
}

Serializer &Serializer::operator << (const std::string & str) {
  return (*this) << AddString(str);
}

