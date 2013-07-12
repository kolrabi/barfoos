#include "common.h"

#ifdef WIN32
#include <windows.h>
#else
#include <time.h>
#endif

struct ProfileFunc {
  std::string name;
  uint64_t calls = 0;
  uint64_t totalTicks = 0;
  uint64_t ticksPerCall = 0;
};

static std::map<std::string, ProfileFunc> funcs;
static std::vector<std::string> funcStack;

static inline uint64_t measure() {
#ifdef WIN32
  LARGE_INTEGER li;
  QueryPerformanceCounter(&li);
  return li.QuadPart;
#else
  timespec t;
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t);
  return t.tv_sec * 1000000000 + t.tv_nsec;
#endif
}

Profile::Profile(const char *func, const char *file, int line) {
  char tmp[512];
  if (funcStack.empty()) {
    snprintf(tmp, sizeof(tmp), "%10s %4d %-42s", file, line, func);
    this->name = tmp;
  } else {
    snprintf(tmp, sizeof(tmp), "%10s %4d %-40s", file, line, func);
    this->name = funcStack.back() + " / \n  " + tmp;
  }
  funcStack.push_back(this->name);
  this->startTick = measure();
}

Profile::~Profile() {
  uint64_t ticks = measure();
  ProfileFunc &func = funcs[this->name];
  func.name = this->name;
  func.calls ++;
  func.totalTicks += ticks - this->startTick;
  func.ticksPerCall = func.totalTicks / func.calls;
  funcStack.pop_back();
}

std::string 
Profile::GetDump() {
  std::stringstream str;
  
  std::vector<ProfileFunc> sortedFuncs;
  for (auto f : funcs) {
    sortedFuncs.push_back(f.second);
  }
  
  std::sort(sortedFuncs.begin(), sortedFuncs.end(), [](const ProfileFunc &a, const ProfileFunc &b){return a.ticksPerCall < b.ticksPerCall;});

  for (auto f : sortedFuncs) {
    char tmp[512];
    snprintf(tmp, sizeof(tmp), "%s %10" PRIu64 " c, %10" PRIu64 " t, %10" PRIu64 " t/c", f.name.c_str(), f.calls, f.totalTicks, f.ticksPerCall);
    str << tmp << std::endl << std::endl;
  }
  
  return str.str();
}

void 
Profile::Dump() {
  std::cerr << GetDump();
}
