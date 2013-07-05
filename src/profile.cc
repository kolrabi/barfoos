#include "common.h"

struct ProfileFunc {
  unsigned long long calls = 0;
  unsigned long long totalTicks = 0;
};

static std::map<std::string, ProfileFunc> funcs;

#if defined(__i386) || defined(__i386__)

static __inline__ unsigned long long rdtsc()
{
    unsigned long long int x;
    asm volatile (".byte 0x0f, 0x31" : "=A" (x));
    return x;
}

#elif defined(__amd64) || defined(__x86_64__)

static __inline__ unsigned long long rdtsc() {
  unsigned long long a, d;
  asm volatile ("rdtsc" : "=a" (a), "=d" (d));
  return (d<<32) | a;
}

#endif

Profile::Profile(const char *func, const char *file, int line) {
  std::stringstream str;
  str << file << ":" << line << " " << func;
  this->name = str.str();
  this->startTick = rdtsc();
}

Profile::~Profile() {
  unsigned long long ticks = rdtsc();
  ProfileFunc &func = funcs[this->name];
  func.calls ++;
  func.totalTicks += ticks - this->startTick;
}

void 
Profile::Dump() {
  for (auto f : funcs) {
    std::cerr << f.first << ": " << f.second.calls << " calls, " << f.second.totalTicks << " ticks, " << (f.second.totalTicks / f.second.calls) << " ticks/call" << std::endl;
  }
}
