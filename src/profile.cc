#include "common.h"

// broken on macos x
#if !defined(NO_PROFILE)

#ifdef WIN32
#include <windows.h>
#else
#include <time.h>
#endif

#include <cstdio>

#include <algorithm>
#include <unordered_map>
#include <sstream>
#include <cinttypes>

#ifndef PRIu64
  #define PRIu64 "I64u"
#endif

struct ProfileFunc {
  std::string name = "";
  uint64_t calls = 0;
  uint64_t totalTicks = 0;
  uint64_t ticksPerCall = 0;
};

static std::unordered_map<std::string, ProfileFunc> funcs;
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

Profile::Profile(const char *func, const char *file, int line) :
  name(""),
  startTick(0)
{
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
#endif

std::string 
Profile::GetDump() {
#ifdef NO_PROFILE
  return "No profiling information available.";
#else 
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
#endif
}

void 
Profile::Dump() {
  Log("%s\n", GetDump().c_str());
}


#ifndef NO_PROFILE

#include <new>

extern FILE *memLog;
static uint64_t firstLog = 0;
static int64_t totalMem = 0;

struct alloc_info {
  int32_t size;
};

void *operator new(size_t size) {
  alloc_info *a = (alloc_info*)malloc(size + sizeof(alloc_info));
  if (!a) {
    throw std::bad_alloc();
  }
  if (!memLog) {
    memLog = fopen("memlog.txt", "wb");
    firstLog = measure();
  } 

  if (memLog) {
    a->size = size;
    totalMem += a->size;
    //printf(memLog, "%llu %u %lld # + %p\n", measure() - firstLog, a->size, totalMem, (char*)a+sizeof(alloc_info));
  }
  return (char*)a + sizeof(alloc_info);
}

void operator delete(void *p) {
  if (!p) return;

  alloc_info *a = (alloc_info*)((char*)p - sizeof(alloc_info));
  if (memLog) {
    totalMem -= a->size;
    //fprintf(memLog, "%llu -%u %lld # -\n", measure() - firstLog, a->size, totalMem);
    /*if (a->size == 8) {
      Log("# %p: ", p);
      for (int i=0; i<a->size; i++)
        Log("%02x ", ((char*)p)[i]&0xFF);
      Log("\n");
    }*/
  }

  free(a);
}

#endif
