#ifndef BARFOOS_FEATURE_H
#define BARFOOS_FEATURE_H

#include "common.h"
#include "util.h"
#include "cell.h"

class Feature;
class Template;
class World;
class Random;

struct FeatureConnection {
  // where can be put a connection to another feature
  IVector3 pos;
  int dir;

  // possible features that can be connected here
  std::vector<std::string> nextFeatures;

  FeatureConnection(const IVector3 &pos, int dir) 
  : pos(pos), dir(dir), resolved(false) {}

  const Feature *GetRandomFeature(const World *world, const IVector3 &pos, Random &r) const;

  void Resolve();
  bool resolved;
};

struct FeatureInstance {
  FeatureInstance(const Feature *feature, IVector3 pos, int dir, int dist) 
  : feature(feature), pos(pos), dir(dir), dist(dist) {}
  
  FeatureInstance() :feature(nullptr), dir(0), dist(0) {}

  const Feature *feature;
  IVector3 pos;
  int dir;
  int dist;
  size_t prevID;
};

enum class SpawnClass {
  Entity, Mob, Item
};

struct FeatureSpawn {
  std::string type;
  SpawnClass spawnClass;
  bool attach;
  float probability;
  Vector3 pos;
};

class Feature {
public:
  Feature() {}
  Feature(FILE *f, const std::string &name);
  ~Feature();
  
  const IVector3 GetSize() const; 
  float GetProbability(const World *world, const IVector3 &pos) const;
  FeatureInstance BuildFeature(World *world, const IVector3 &pos, int dir, int dist, size_t id) const;
  void SpawnEntities(World *world, const IVector3 &pos) const;
  
  const std::vector<FeatureConnection> &GetConnections() const { return conns; }

  const FeatureConnection *GetRandomConnection(Random &r) const;
  const FeatureConnection *GetRandomConnection(int dir, Random &r) const;
  
  const std::string &GetName() const { return name; }
  const std::string &GetGroup() const { return group; }

  void ResolveConnections();

protected:

  std::vector<FeatureConnection> conns;
  std::vector<FeatureSpawn> spawns;
  std::string name;
  std::string group;

  std::vector<Cell> cells;
  std::vector<bool> defaultMask;
  IVector3 size; 
  
  int minLevel, maxLevel;
  float maxProbability;

  size_t minY;
};

void LoadFeatures();
const Feature *getFeature(const std::string &);

#endif

