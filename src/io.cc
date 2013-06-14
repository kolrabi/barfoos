#include "common.h"
#include "util.h"

#include <sys/stat.h>
#include <dirent.h>

static const std::string assetPrefix("../assets/");

FILE *openAsset(const std::string &path) {
  FILE *f;
  f = fopen((assetPrefix + path).c_str(), "rb");
  if (f) {
    std::cerr << "reading " << (assetPrefix + path) << std::endl;
  } else {
    std::cerr << "could not open " << (assetPrefix + path) << std::endl;
  }
  return f;
}

static size_t getFileSize(const std::string &name) {
  struct stat st;
  int res = stat((assetPrefix + name).c_str(), &st);
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
    return "";
  }
  text[size] = 0;
  std::string str = text;
  delete[]text;
  return str;
}

time_t getFileChangeTime(const std::string &name) {
  struct stat st;
  int res = stat((assetPrefix + name).c_str(), &st);
  if (res < 0) return 0;
  return st.st_mtime;
}

std::vector<std::string> findAssets(const std::string &type) {
  std::vector<std::string> assets;

  DIR *dir = opendir((assetPrefix+type).c_str());

  dirent *ent;
  while((ent = readdir(dir)) != nullptr) { 
    assets.push_back(ent->d_name);
  }

  closedir(dir);

  return assets;  
}

