#ifndef BARFOOS_IO_H
#define BARFOOS_IO_H

#include <cstdio>
#include <vector>

// asset management
FILE *openAsset(const std::string &name);
std::string loadAssetAsString(const std::string &name);
std::vector <std::string> findAssets(const std::string &type);

// file management
FILE *createUserFile(const std::string &name);
FILE *openUserFile(const std::string &name);
time_t getFileChangeTime(const std::string &name);

#endif
