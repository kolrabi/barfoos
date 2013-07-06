#include <time.h>
#include "common.h"

struct ProfileFunc {
  std::string name;
  unsigned long long calls = 0;
  unsigned long long totalTicks = 0;
  unsigned long long ticksPerCall = 0;
};

static std::map<std::string, ProfileFunc> funcs;
/*
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
*/


static inline unsigned long long measure() {
  timespec t;
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t);
  return t.tv_sec * 1000000000 + t.tv_nsec;
}

Profile::Profile(const char *func, const char *file, int line) {
  std::stringstream str;
  str << file << ":" << line << " " << func;
  this->name = str.str();
  this->startTick = measure();
}

Profile::~Profile() {
  unsigned long long ticks = measure();
  ProfileFunc &func = funcs[this->name];
  func.name = this->name;
  func.calls ++;
  func.totalTicks += ticks - this->startTick;
  func.ticksPerCall = func.totalTicks / func.calls;
}

void 
Profile::Dump() {
  std::vector<ProfileFunc> sortedFuncs;
  for (auto f : funcs) {
    sortedFuncs.push_back(f.second);
  }
  
  std::sort(sortedFuncs.begin(), sortedFuncs.end(), [](const ProfileFunc &a, const ProfileFunc &b){return a.ticksPerCall < b.ticksPerCall;});

  for (auto f : sortedFuncs) {
    std::cerr << f.name << ": " << f.calls << " calls, " << f.totalTicks << " ticks, " << f.ticksPerCall << " ticks/call" << std::endl;
  }
}
