#include "common.h"
#include "util.h"

#include <sys/stat.h>
#include <dirent.h>

#include <cstdio>

static const std::vector<const char *> assetPrefix { "../assets/", DATA_PATH "/" };

static std::string getAssetPath(const std::string &name) {
  for (std::string prefix : assetPrefix) {
    std::string fullPath = prefix + name;
    struct stat st;
    int res = stat(fullPath.c_str(), &st);
    if (res >= 0) return fullPath;
  }
  return "";
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

