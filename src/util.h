#ifndef BARFOOS_UTIL_H
#define BARFOOS_UTIL_H

#include "common.h"

FILE *openAsset(const std::string &name);
std::string loadAssetAsString(const std::string &name);
time_t getFileChangeTime(const std::string &name);

std::vector <std::string> findAssets(const std::string &type);
float Wave(float x, float z, float t, float a = 0.2);

std::vector<std::string> Tokenize(const char *line);
size_t ParseSidesMask(const std::string &str);

int saveImage(const std::string &fileName, size_t w, size_t h, const uint8_t *rgb);

std::vector <std::string> findAssets(const std::string &type);

#endif

