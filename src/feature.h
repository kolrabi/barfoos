#ifndef BARFOOS_FEATURE_H
#define BARFOOS_FEATURE_H

#include "common.h"
#include "util.h"
#include "cell.h"

#include <unordered_map>

class Feature;

struct FeatureConnection {
  // where can be put a connection to another feature
  IVector3 pos;
  int dir;
  size_t id;

  // possible features that can be connected here
  std::unordered_map<std::string, float> nextFeatures;
  
  bool resolved;

  FeatureConnection(const IVector3 &pos, int dir, size_t id) : 
    pos(pos), 
    dir(dir), 
    id(id), 
    nextFeatures(),
    resolved(false)
  {}

  const Feature *GetRandomFeature(RunningState &state, const IVector3 &pos) const;

  void Resolve();
};

struct FeatureInstance {

  const Feature *feature;
  IVector3 pos;
  int dir;
  int dist;
  size_t featureID;
  size_t prevID;
  
  FeatureInstance(const Feature *feature, IVector3 pos, int dir, int dist, size_t id) : 
    feature(feature), 
    pos(pos), 
    dir(dir), 
    dist(dist), 
    featureID(id), 
    prevID(~0UL) 
  {}
  
  FeatureInstance() : 
    feature(nullptr), 
    pos(),
    dir(0), 
    dist(0), 
    featureID(~0UL), 
    prevID(~0UL) 
  {}
};

struct FeatureSpawn {
  std::string type;
  SpawnClass spawnClass;
  int attach;
  float probability;
  Vector3 pos;

  FeatureSpawn();
};

struct FeatureReplacement {
  size_t conn;
  char orig, replace;
};

struct FeatureCharDef {
  std::string type;
  float top[4]; 
  float bot[4];
  bool botRev, topRev;
  bool revRand;
  bool lockCell;
  bool ignoreLock;
  bool ignoreWrite;
  bool onlydefault;
  
  FeatureCharDef() :
    type(""),
    top{ 1, 1, 1, 1 },
    bot{ 0, 0, 0, 0 },
    botRev(false),
    topRev(false),
    revRand(true),
    lockCell(true),
    ignoreLock(false),
    ignoreWrite(false),
    onlydefault(false)
  {}
};

class Feature final {
public:
  Feature();
  Feature(FILE *f, const std::string &name);
  ~Feature();
  
  const IVector3 GetSize() const; 
  float GetProbability(const RunningState &state, const IVector3 &pos) const;
  FeatureInstance BuildFeature(RunningState &state, World &world, const IVector3 &pos, int dir, int dist, size_t id, const FeatureConnection *conn) const;
  void SpawnEntities(RunningState &state, const IVector3 &pos) const;
  
  const std::vector<FeatureConnection> &GetConnections() const { return conns; }

  const FeatureConnection *GetRandomConnection(RunningState &state) const;
  const FeatureConnection *GetRandomConnection(int dir, RunningState &state) const;
  
  const std::string &GetName()  const { return name;  }
  const std::string &GetGroup() const { return group; }

  void ResolveConnections();
  void ReplaceChars(RunningState &state, World &world, const IVector3 &pos, size_t connId, size_t featureId) const;

protected:

  std::unordered_map<char, FeatureCharDef> defs;
  std::vector<FeatureConnection> conns;
  std::vector<FeatureSpawn> spawns;
  std::vector<FeatureReplacement> replacements;
  std::string name;
  std::string group;

  std::vector<Cell> cells;
  std::vector<bool> defaultMask;
  std::vector<char> chars;
  IVector3 size; 
  
  int minLevel, maxLevel;
  float maxProbability;

  size_t minY;
  
  void ReplaceChars(const FeatureReplacement &r, std::vector<char> &chars) const;
};

void LoadFeatures();
const Feature *getFeature(const std::string &);

#endif

