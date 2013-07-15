#ifndef BARFOOS_PROFILE_H
#define BARFOOS_PROFILE_H

/** Simple profiling class. */
class Profile {
public:

  Profile(const char *func, const char *file, int line);
  ~Profile();
  
  static void Dump();
  static std::string GetDump();
  
private:
  
  std::string name;
  unsigned long long startTick;
};

#define PROFILE() Profile __profile(__PRETTY_FUNCTION__, __FILE__, __LINE__)
#define PROFILE_NAMED(n) Profile __profile(n, __FILE__, __LINE__)

#endif

