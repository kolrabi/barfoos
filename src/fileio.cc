#include "common.h"

#include "fileio.h"

#include <cstring>
#include <cerrno>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <Shlobj.h>
#endif

#include <sys/stat.h>
#include <dirent.h>

static const std::vector<const char *> assetPrefix = { 
  "../assets/", 
  "assets/",
  DATA_PATH "/" 
};

static std::string getAssetPath(const std::string &name) {
  for (std::string prefix : assetPrefix) {
    std::string fullPath = prefix + name;
    struct stat st;
    int res = stat(fullPath.c_str(), &st);
    if (res >= 0) return fullPath;
  }
  return "";
}

std::string getUserPath(const std::string &name) {
  std::string base = "./";
#ifdef WIN32
  TCHAR szPath[MAX_PATH];
  if (SUCCEEDED(SHGetFolderPath(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, szPath))) 
    base = szPath;

  for (auto &c:base) if (c == '\\') c = '/';
#else
  if (getenv("HOME"))
    base = getenv("HOME");
#endif
  if (base.back() != '/') base += "/";
  
  // Log("%s\n", (base + "." + PACKAGE_NAME + "/" + name).c_str());

  return base + "." + PACKAGE_NAME + "/" + name;
}

static bool fileIsDir(const std::string &path) {
  struct stat st;
  int res = stat(path.c_str(), &st);
  if (res < 0) return false;
  return S_ISDIR(st.st_mode);
}

FILE *openAsset(const std::string &name) {
  std::string fullPath = getAssetPath(name);
  
  FILE *f = fopen(fullPath.c_str(), "rb");
  if (!f) {
    Log("Could not open asset file '%s'\n", name.c_str());
  }
  return f;
}

static size_t getFileSize(const std::string &name) {
  std::string fullPath = getAssetPath(name);
  
  struct stat st;
  int res = stat(fullPath.c_str(), &st);
  if (res < 0) return 0;
  return st.st_size;
}

std::string loadAssetAsString(const std::string &name) {
  size_t size = getFileSize(name);
  if (size == 0) return "";
  
  FILE *f = openAsset(name);
  if (!f) return "";
  
  char *text = new char[size + 1];
  if (fread(text, size, 1, f) != 1) {
    delete[]text;
    fclose(f);
    return "";
  }
  text[size] = 0;
  std::string str = text;
  delete[]text;
  fclose(f);
  return str;
}

time_t getFileChangeTime(const std::string &name) {
  std::string fullPath = getAssetPath(name);
  
  struct stat st;
  int res = stat(fullPath.c_str(), &st);
  if (res < 0) return 0;
  return st.st_mtime;
}

std::vector<std::string> findAssets(const std::string &type) {
  std::string fullPath = getAssetPath(type);
  std::vector<std::string> assets;

  DIR *dir = opendir(fullPath.c_str());

  dirent *ent;
  while((ent = readdir(dir)) != nullptr) { 
    if (ent->d_name[0] != '.' && !fileIsDir(fullPath+"/"+ent->d_name))
      assets.push_back(ent->d_name);
  }

  closedir(dir);

  return assets;  
}

void makePath(const std::string &path) {
  size_t pos = path.find_last_of('/');
  if (pos == std::string::npos) return;

#ifdef WIN32
  int res = mkdir(path.substr(0, pos).c_str());
#else
  int res = mkdir(path.substr(0, pos).c_str(), 0700);
#endif
  if (res) perror(path.substr(0,pos).c_str());
}

FILE *createUserFile(const std::string &name) {
  std::string path = getUserPath(name);
  makePath(path);
  
  FILE *file = fopen(path.c_str(), "wb");
  if (!file) perror(path.c_str());
  return file;
}

bool 
createUserStream(const std::string &name, std::fstream &out) {
  std::string path = getUserPath(name);
  makePath(path);
  out.open(path, std::ios::binary | std::ios::out | std::ios::trunc);
  return out.is_open();
}

FILE *openUserFile(const std::string &name) {
  std::string path = getUserPath(name);
  FILE *file = fopen(path.c_str(), "rb");
  if (!file) perror(path.c_str());
  return file;
}
