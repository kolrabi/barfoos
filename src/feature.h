#ifndef BARFOOS_FEATURE_H
#define BARFOOS_FEATURE_H

#include "common.h"
#include "util.h"
#include "cell.h"

#include <map>

class Feature;

struct FeatureConnection {
  // where can be put a connection to another feature
  IVector3 pos;
  int dir;
  ID id;

  // possible features that can be connected here
  std::map<std::string, float> nextFeatures;
  
  bool resolved;

  FeatureConnection(const IVector3 &pos, int dir, ID id) : 
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
  ID featureID;
  ID prevID;
  
  FeatureInstance(const Feature *feature, IVector3 pos, int dir, int dist, ID id) : 
    feature(feature), 
    pos(pos), 
    dir(dir), 
    dist(dist), 
    featureID(id), 
    prevID(InvalidID) 
  {}
  
  FeatureInstance() : 
    feature(nullptr), 
    pos(),
    dir(0), 
    dist(0), 
    featureID(InvalidID), 
    prevID(InvalidID) 
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
  float topNoise;
  float bottomNoise;
  float topFreq;
  float bottomFreq;
  float topDisplace;
  float bottomDisplace;
  
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
    onlydefault(false),
    topNoise(0.0),
    bottomNoise(0.0),
    topFreq(0.0),
    bottomFreq(0.0),
    topDisplace(0.0),
    bottomDisplace(0.0)
  {}
  
  void Rotate() {
    float tmp;
    tmp = top[0];
    top[0] = top[1];
    top[1] = top[2];
    top[2] = top[3];
    top[3] = tmp;
    tmp = bot[0];
    bot[0] = bot[1];
    bot[1] = bot[2];
    bot[2] = bot[3];
    bot[3] = tmp;
    botRev = !botRev;
    topRev = !topRev;
  }
};

class Feature final {
public:
  Feature();
  Feature(FILE *f, const std::string &name);
  ~Feature();
  
  const IVector3 GetSize() const; 
  float GetProbability(const RunningState &state, const IVector3 &pos) const;
  FeatureInstance BuildFeature(RunningState &state, World &world, const IVector3 &pos, int dir, int dist, ID id, const FeatureConnection *conn, ID prevId) const;
  void SpawnEntities(RunningState &state, const IVector3 &pos) const;
  
  const std::vector<FeatureConnection> &GetConnections() const { return conns; }

  const FeatureConnection *GetRandomConnection(RunningState &state) const;
  const FeatureConnection *GetRandomConnection(int dir, RunningState &state) const;
  
  const std::string &GetName()  const { return name;  }
  const std::vector<std::string> &GetGroups() const { return groups; }
  const std::string &GetDecoGroup()  const { return decoGroup;  }

  void ResolveConnections();
  void ReplaceChars(RunningState &state, World &world, const IVector3 &pos, ID connId, ID featureId) const;
  
  Feature Rotate();

protected:

  std::vector<Feature> variants;

  std::map<char, FeatureCharDef> defs;
  std::vector<FeatureConnection> conns;
  std::vector<FeatureSpawn> spawns;
  std::vector<FeatureReplacement> replacements;
  std::string name;
  std::vector<std::string> groups;
  
  std::string decoGroup = "misc";

  std::vector<Cell> cells;
  std::vector<bool> defaultMask;
  std::vector<char> chars;
  IVector3 size; 
  
  int minLevel, maxLevel;
  float maxProbability;

  size_t minY;
  
  bool useLastId;
  bool noRotate;
  
  void ReplaceChars(const FeatureReplacement &r, std::vector<char> &chars) const;
  
  friend void LoadFeatures();
  friend const Feature *FeatureConnection::GetRandomFeature(RunningState &state, const IVector3 &pos) const;
};

void LoadFeatures();
const Feature *getFeature(const std::string &);

#endif

