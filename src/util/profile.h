#ifndef BARFOOS_PROFILE_H
#define BARFOOS_PROFILE_H

#ifdef MACOSX
#define NO_PROFILE 1
#endif

#define NO_PROFILE 1

#ifdef NO_PROFILE

// broken

#define PROFILE()
#define PROFILE_NAMED(n)

class Profile {
public:

  ~Profile() {}

  static void Dump();
  static std::string GetDump();

private:

  //std::string name;
  //unsigned long long startTick;
};

#else

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

#endif

