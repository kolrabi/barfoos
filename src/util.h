#ifndef BARFOOS_UTIL_H
#define BARFOOS_UTIL_H

#include "common.h"
#include <cstdio>
#include <string>

#include <sys/time.h>
#include <random>

extern size_t guiActive;
extern int screenWidth, screenHeight;
extern int virtualScreenWidth, virtualScreenHeight;

FILE *openAsset(const std::string &name);
std::string loadAssetAsString(const std::string &name);
time_t getFileChangeTime(const std::string &name);

unsigned int loadTexture(const std::string &name, unsigned int tex = 0);
void updateTextures();
std::vector <std::string> findAssets(const std::string &type);

void drawUnitCube();
void drawVerticalBillboard(const Vector3 &pos, float w, float h, unsigned int tex, float u=0, float uw=1, float ofsX = 0, float ofsY = 0);  
void drawBillboard(const Vector3 &pos, float w, float h, unsigned int tex, float u=0, float uw=1, float ofsX = 0, float ofsY = 0);  
void drawAABB(const AABB &aabb);
float Wave(float x, float z, float t, float a = 0.2, float o = 0.5);

void viewGUI();
void drawIcon(const Point &center, const Point &size, unsigned int tex, float u=0, float uw=1);  

Point alignBottomLeftScreen(const Point &size, int padding = 0);
Point alignBottomRightScreen(const Point &size, int padding = 0);
Point alignTopLeftScreen(const Point &size, int padding = 0);
Point alignTopRightScreen(const Point &size, int padding = 0);

std::vector<std::string> Tokenize(const char *line);

#endif

