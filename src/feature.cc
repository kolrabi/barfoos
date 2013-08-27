#include "common.h"
#include "feature.h"
#include "world.h"
#include "cell.h"
#include "random.h"
#include "runningstate.h"
#include "simplex.h"
#include "entity.h"

#include <unordered_map>
#include <algorithm>
#include "weighted_map.h"

FeatureSpawn::FeatureSpawn() :
  type(""),
  spawnClass(SpawnClass::EntityClass),
  attach(0),
  probability(0.0),
  pos(0,0,0)
{}

static std::unordered_map<std::string, Feature> allFeatures;

const Feature *getFeature(const std::string &name) {
  for (auto &f:allFeatures) {
    if (f.first == name) return &f.second;
  }
  Log("Feature of type '%s' not found\n", name.c_str());
  return nullptr;
}

Feature::Feature() :
  defs(),
  conns(),
  spawns(),
  replacements(),
  name("<undefined>"),
  groups(),
  cells(0),
  defaultMask(0),
  chars(0),
  size(0,0,0),
  minLevel(0),
  maxLevel(0),
  maxProbability(0.0),
  minY(0),
  useLastId(false),
  noRotate(false)
{}

Feature::Feature(FILE *f, const std::string &name) :
  defs(),
  conns(),
  spawns(),
  replacements(),
  name(name),
  groups(),
  cells(0),
  defaultMask(0),
  chars(0),
  size(0,0,0),
  minLevel(0),
  maxLevel(-1),
  maxProbability(1.0),
  minY(0),
  useLastId(false),
  noRotate(false)
{
  char line[256];
  char lastDef = 0;
  
  this->groups.push_back(name);
  
  while(fgets(line, 256, f) && !feof(f)) {
    std::vector<std::string> tokens = Tokenize(line);
    if (tokens.empty()) continue;
    
    for (auto &c:tokens[0]) c = ::tolower(c);
    
    if (tokens[0] == "size") {
      this->size = IVector3(
        std::atoi(tokens[1].c_str()), 
        std::atoi(tokens[2].c_str()), 
        std::atoi(tokens[3].c_str())
      );
      cells = std::vector<Cell>(size.x * size.y * size.z);
      defaultMask = std::vector<bool>(size.x*size.y*size.z, false);
      chars = std::vector<char>(size.x * size.y * size.z);
    } else if (tokens[0] == "level") {
      this->minLevel = std::atoi(tokens[1].c_str());
      this->maxLevel = std::atoi(tokens[2].c_str());
      this->maxProbability = std::atof(tokens[3].c_str());
    } else if (tokens[0] == "group") {
      this->groups.push_back(tokens[1]);
    } else if (tokens[0] == "deco") {
      this->decoGroup = tokens[1];
    } else if (tokens[0] == "above") {
      this->minY = std::atoi(tokens[1].c_str());
    } else if (tokens[0] == "uselastid") {
      this->useLastId = true;
    } else if (tokens[0] == "conn") {
      IVector3 pos(std::atoi(tokens[1].c_str()), std::atoi(tokens[2].c_str()), std::atoi(tokens[3].c_str()));
      int dir = std::atoi(tokens[4].c_str());
      conns.push_back(FeatureConnection(pos, dir, conns.size()));
    } else if (tokens[0] == "onconnreplace") {
      FeatureReplacement r;
      r.conn = conns.size()-1;
      r.orig = tokens[1][0];
      r.replace = tokens[2][0];
      replacements.push_back(r);
    } else if (tokens[0] == "next") {
      conns.back().nextFeatures[tokens[1]] = 1.0f;
    } else if (tokens[0] == "nextp") {
      conns.back().nextFeatures[tokens[1]] = std::atof(tokens[2].c_str());
    } else if (tokens[0] == "def") {
      lastDef = tokens[1][0];
      defs[lastDef] = FeatureCharDef();
      if (tokens.size() > 2) defs[lastDef].type = tokens[2];
    } else if (tokens[0] == "cell") {
      defs[lastDef].type = tokens[1];
    } else if (tokens[0] == "top") {
      defs[lastDef].top[0] = std::atof(tokens[1].c_str());
      defs[lastDef].top[1] = std::atof(tokens[2].c_str());
      defs[lastDef].top[2] = std::atof(tokens[3].c_str());
      defs[lastDef].top[3] = std::atof(tokens[4].c_str());
    } else if (tokens[0] == "bottom") {
      defs[lastDef].bot[0] = std::atof(tokens[1].c_str());
      defs[lastDef].bot[1] = std::atof(tokens[2].c_str());
      defs[lastDef].bot[2] = std::atof(tokens[3].c_str());
      defs[lastDef].bot[3] = std::atof(tokens[4].c_str());
    } else if (tokens[0] == "override") {
      defs[lastDef].ignoreLock = true;
    } else if (tokens[0] == "nolock") {
      defs[lastDef].lockCell = false;
    } else if (tokens[0] == "nowrite") {
      defs[lastDef].ignoreWrite = true;
    } else if (tokens[0] == "onlydefault") {
      defs[lastDef].onlydefault = true;
    } else if (tokens[0] == "brev") {
      defs[lastDef].botRev = true;
      defs[lastDef].revRand = false;
    } else if (tokens[0] == "trev") {
      defs[lastDef].topRev = true;
      defs[lastDef].revRand = false;
    } else if (tokens[0] == "srev") {
      defs[lastDef].sideRev = true;
      defs[lastDef].revRand = false;
    } else if (tokens[0] == "topnoise") {
      defs[lastDef].topNoise = std::atof(tokens[1].c_str());
      defs[lastDef].topFreq = std::atof(tokens[2].c_str());
    } else if (tokens[0] == "bottomnoise") {
      defs[lastDef].bottomNoise = std::atof(tokens[1].c_str());
      defs[lastDef].bottomFreq = std::atof(tokens[2].c_str());
    } else if (tokens[0] == "topdisplace") {
      defs[lastDef].topDisplace = std::atof(tokens[1].c_str());
    } else if (tokens[0] == "bottomdisplace") {
      defs[lastDef].bottomDisplace = std::atof(tokens[1].c_str());
    } else if (tokens[0] == "mob") {
      FeatureSpawn spawn;
      spawn.spawnClass = SpawnClass::MobClass;
      spawn.probability = std::atof(tokens[1].c_str());
      spawn.type = tokens[2];
      spawn.attach = std::atoi(tokens[3].c_str());
      spawn.pos = Vector3(std::atof(tokens[4].c_str()), std::atof(tokens[5].c_str()), std::atof(tokens[6].c_str()));
      this->spawns.push_back(spawn);
    } else if (tokens[0] == "entity") {
      FeatureSpawn spawn;
      spawn.spawnClass = SpawnClass::EntityClass;
      spawn.probability = std::atof(tokens[1].c_str());
      spawn.type = tokens[2];
      spawn.attach = std::atoi(tokens[3].c_str());
      spawn.pos = Vector3(std::atof(tokens[4].c_str()), std::atof(tokens[5].c_str()), std::atof(tokens[6].c_str()));
      this->spawns.push_back(spawn);
    } else if (tokens[0] == "item") {
      FeatureSpawn spawn;
      spawn.spawnClass = SpawnClass::ItemEntityClass;
      spawn.probability = std::atof(tokens[1].c_str());
      spawn.type = tokens[2];
      spawn.attach = std::atoi(tokens[3].c_str());
      spawn.pos = Vector3(std::atof(tokens[4].c_str()), std::atof(tokens[5].c_str()), std::atof(tokens[6].c_str()));
      this->spawns.push_back(spawn);
    } else if (tokens[0] == "norotate") {
      this->noRotate = true;
    } else if (tokens[0] == "slice") {
      size_t y0 = std::atof(tokens[1].c_str());
      size_t y1 = std::atof(tokens[2].c_str());
      for (size_t z=0; z<size.z; z++) {
        char *p = fgets(line, 256, f);
        if (!p) break;
        for (size_t x=0; x<size.x; x++) {
          FeatureCharDef &def = defs[line[x]];
          for (size_t y=y0; y<=y1; y++) {
            Cell cell(def.type);
            
            float dispT = simplexNoise( Vector3(x,y,z) + 0.5 ) * def.topDisplace;
            
            float ofsT[4] = {
              def.topNoise * simplexNoise( Vector3(x,y,z) * def.topFreq ) + dispT,
              def.topNoise * simplexNoise( Vector3(x,y,z+1) * def.topFreq ) + dispT,
              def.topNoise * simplexNoise( Vector3(x+1,y,z+1) * def.topFreq ) + dispT,
              def.topNoise * simplexNoise( Vector3(x+1,y,z) * def.topFreq ) + dispT
            };
            
            float dispB = simplexNoise( Vector3(x,y,z) + 0.5 ) * def.bottomDisplace;
            
            float ofsB[4] = {
              def.bottomNoise * simplexNoise( Vector3(x,y,z) * def.bottomFreq ) + dispB,
              def.bottomNoise * simplexNoise( Vector3(x,y,z+1) * def.bottomFreq ) + dispB,
              def.bottomNoise * simplexNoise( Vector3(x+1,y,z+1) * def.bottomFreq ) + dispB,
              def.bottomNoise * simplexNoise( Vector3(x+1,y,z) * def.bottomFreq ) + dispB
            };
            
            cell.SetYOffsets(def.top[0]+ofsT[0],def.top[1]+ofsT[1],def.top[2]+ofsT[2],def.top[3]+ofsT[3]);
            cell.SetYOffsetsBottom(def.bot[0]+ofsB[0],def.bot[1]+ofsB[1],def.bot[2]+ofsB[2],def.bot[3]+ofsB[3]);
            cell.SetOrder(def.topRev, def.botRev);
            cell.SetLocked(def.lockCell);
            cell.SetIgnoreLock(def.ignoreLock);
            cell.SetIgnoreWrite(def.ignoreWrite);
            
            cells[x+size.x*(y+size.y*z)] = cell;
            
            defaultMask[x+size.x*(y+size.y*z)] = def.onlydefault;
            chars[x+size.x*(y+size.y*z)] = line[x];
          }
        }
      }
    }
    else if (tokens[0] != "") {
      Log("Ignoring unknown feature property: %s\n", tokens[0].c_str());
    }
  }

}

Feature::~Feature() {
}

const IVector3 Feature::GetSize() const {
  return size;
}

float Feature::GetProbability(const RunningState &state, const IVector3 &pos) const {
  if (pos.y < this->minY) {
    return 0;
  }

  int level = state.GetLevel();
  
  if (level < this->minLevel) return 0;
  if (this->maxLevel < this->minLevel) return this->maxProbability;

  // make highest chance right between min and max level
  float levelFrac = (level-minLevel)/(float)(maxLevel-minLevel+1);
  return maxProbability * std::sin(Const::pi*levelFrac);
}

FeatureInstance Feature::BuildFeature(RunningState &state, World &world, const IVector3 &pos, int dir, int dist, ID id, const FeatureConnection *conn, ID prevId) const {
  if (this->useLastId) id = prevId;
  
  for (size_t z=0; z<size.z; z++) {
    for (size_t y=0; y<size.y; y++) {
      for (size_t x=0; x<size.x; x++) { 
        if (defaultMask[x+size.x*(y+size.y*z)] && world.IsChecking()) continue;
        if (defaultMask[x+size.x*(y+size.y*z)] && !world.IsDefault(pos+IVector3(x,y,z))) continue;
        world.SetCell(pos+IVector3(x,y,z), cells[x+size.x*(y+size.y*z)]).SetFeatureID(id);
      }
    }
  }
  if (conn) {
    this->ReplaceChars(state, world, pos, conn->id, id);
  }
  return FeatureInstance(this, pos, dir, dist+1, id);
}

void Feature::ReplaceChars(RunningState &state, World &world, const IVector3 &pos, ID connId, ID featureId) const {
  std::vector<char> repchars = this->chars;
  for (const FeatureReplacement &r : replacements) {
    if (r.conn == connId) {
      this->ReplaceChars(r, repchars);
    }
  }
  
  for (size_t z=0; z<size.z; z++) {
    for (size_t y=0; y<size.y; y++) {
      for (size_t x=0; x<size.x; x++) { 
        size_t index = x+size.x*(y+size.y*z);
        char co = this->chars[index];
        char cr = repchars[index];
        
        if (co == cr) continue;
        
        const FeatureCharDef &def = this->defs.find(cr)->second;
        
        Cell cell = Cell(def.type);
        cell.SetYOffsets(def.top[0],def.top[1],def.top[2],def.top[3]);
        cell.SetYOffsetsBottom(def.bot[0],def.bot[1],def.bot[2],def.bot[3]);
        if (def.revRand) {
          cell.SetOrder(state.GetRandom().Integer(2), state.GetRandom().Integer(2));
        } else {
          cell.SetOrder(def.topRev, def.botRev);
        }
        cell.SetReversedSides(def.sideRev);
        cell.SetLocked(def.lockCell);
        cell.SetIgnoreLock(true);
        cell.SetIgnoreWrite(def.ignoreWrite);
        world.SetCell(pos+IVector3(x,y,z), cell).SetFeatureID(featureId);
      }
    }
  }
  
  if (featureId > 1)
  for (size_t z=0; z<size.z; z++) {
    for (size_t y=0; y<size.y; y++) {
      for (size_t x=0; x<size.x; x++) { 
        Cell &cell = world.GetCell(pos+IVector3(x,y,z));
        if (cell.GetInfo().lockedChance && state.GetRandom().Chance(cell.GetInfo().lockedChance)) {
          state.LockCell(cell);
        }
      }
    }
  }
}

void Feature::ReplaceChars(const FeatureReplacement &r, std::vector<char> &chars) const {
  for (size_t i=0; i<chars.size(); i++) {
    if (chars[i] == r.orig) chars[i] = r.replace;
  }
}

void Feature::SpawnEntities(RunningState &state, const IVector3 &pos) const {
  for (const FeatureSpawn &spawn : spawns) {
    if (state.GetRandom().Chance(spawn.probability)) {
      Entity *entity = nullptr;
      
      std::string type = spawn.type;
      if (type[0] == '$') {
        weighted_map<std::string> types;
        for (const std::string &t : GetEntitiesInGroup(type.substr(1))) {
          types[t] = GetEntityProbability(t, state.GetLevel());
        }
        if (type.size() == 0) continue;
        type = types.select(state.GetRandom().Float01());
      }
      
      entity = Entity::Create(type);
      
      Vector3 spawnPos = Vector3(pos)+spawn.pos;
      if (spawn.attach) {
        spawnPos = spawnPos + Vector3(0.5,0.5,0.5);
        IVector3 cellPos = spawnPos;
        Side side = Side::InvalidSide;
        
        if (spawn.attach == -2) {
          side = Side::Down;
          spawnPos.y = cellPos.y + entity->GetAABB().extents.y + 0.001;
        } else if (spawn.attach == 2) {
          side = Side::Up;
          spawnPos.y = cellPos.y + 1-entity->GetAABB().extents.y - 0.001;
        } else if (spawn.attach == 1) {
          side = Side::Right;
          spawnPos.x = cellPos.x + 1-entity->GetAABB().extents.x - 0.001;
        } else if (spawn.attach == -1) {
          side = Side::Left;
          spawnPos.x = cellPos.x + entity->GetAABB().extents.x + 0.001;
        } else if (spawn.attach == 3) {
          side = Side::Forward;
          spawnPos.z = cellPos.z + 1-entity->GetAABB().extents.z - 0.001;
        } else if (spawn.attach == -3) {
          side = Side::Backward;
          spawnPos.z = cellPos.z + entity->GetAABB().extents.z + 0.001;
        }
        
        if (!state.GetWorld().GetCell(cellPos[side]).IsSolid()) {
          delete entity;
          continue;
        }
      }
      
      entity->SetPosition(spawnPos);
      
      state.AddEntity(entity);
    }
  }
}

void FeatureConnection::Resolve() {
  if (!this->resolved) {
    this->resolved = true;

    auto iter = nextFeatures.begin();
    while(iter != nextFeatures.end()) {   
      if (iter->first[0] == '$') {
        std::string group = iter->first.substr(1);
        float prob = iter->second;
        iter = nextFeatures.erase(iter);
        for (auto f : allFeatures) {
          const std::vector<std::string> &groups = f.second.GetGroups();
          if (std::find(groups.begin(), groups.end(), group) != groups.end()) {
            if (nextFeatures.find(f.first) != nextFeatures.end()) {
              nextFeatures[f.first] *= prob;
            } else {
              nextFeatures.insert({f.first, prob});
              iter = nextFeatures.begin();
            }
          }
        }
      } else { 
        iter++;
      }
    } 
  }
} 

const Feature *FeatureConnection::GetRandomFeature(RunningState &state, const IVector3 &pos) const {
  if (nextFeatures.empty()) return nullptr;
  
  weighted_map<const Feature *> wm;
  
  for (auto fname : nextFeatures) {
    const Feature *f = getFeature(fname.first);
    if (!f) continue;
    float w = std::abs(f->GetProbability(state, pos+this->pos)*fname.second);
    if (w <= 0.0) continue;
    
    wm[f] = w;
  }
  
  const Feature *feature = wm.select(state.GetRandom().Float01());
  if (!feature) return nullptr;
  
  size_t variant = state.GetRandom().Integer(4);
  if (variant >= feature->variants.size()) return feature;
  return &feature->variants[variant];
}

const FeatureConnection *
Feature::GetRandomConnection(
  RunningState &state
) const {
  if (conns.empty()) return nullptr;
  return &conns[state.GetRandom().Integer(conns.size())];
}

const FeatureConnection *
Feature::GetRandomConnection(
  int dir, 
  RunningState &state
) const {
  std::vector<const FeatureConnection *> cs;
  for (const FeatureConnection &c : conns) {
    if (c.dir == dir) cs.push_back(&c);
  }
  if (cs.empty()) return nullptr;
  return cs[state.GetRandom().Integer(cs.size())];
}

void
Feature::ResolveConnections() {
  for (FeatureConnection &conn : conns) {
    conn.Resolve();
  }
  
  for (auto &v:variants) {
    v.ResolveConnections();
  }
}

Feature
Feature::Rotate() {
  Feature feature = *this;
  if (this->noRotate) return feature;
  
  feature.size = IVector3(size.z, size.y, size.x);
  for (size_t y=0; y<size.y; y++) {
    for (size_t x=0; x<size.x; x++) {
      for (size_t z=0; z<size.z; z++) {
        IVector3 from(x,y,z);
        IVector3 to(from.Rotate(size));
        size_t fromIndex = from.x+        size.x*(y+        size.y*from.z);
        size_t toIndex   = to.x  +feature.size.x*(y+feature.size.y*to.z);
        feature.cells[toIndex] = cells[fromIndex];
        feature.cells[toIndex].Rotate();
        feature.defaultMask[toIndex] = defaultMask[fromIndex];
        feature.chars[toIndex] = chars[fromIndex];
      }
    }
  }
  
  for (auto &conn:feature.conns) {
    conn.pos = conn.pos.Rotate(size);
    switch(conn.dir) {
      case  1: conn.dir =  3; break;
      case -1: conn.dir = -3; break;
      case  3: conn.dir = -1; break;
      case -3: conn.dir =  1; break;
      default: break;
    }
  }
  
  for (auto &spawn:feature.spawns) {
    if (spawn.attach) {
      spawn.pos = Vector3(size.z-spawn.pos.z-1, spawn.pos.y, spawn.pos.x);
    } else {
      spawn.pos = Vector3(size.z-spawn.pos.z, spawn.pos.y, spawn.pos.x);
    }
    
    switch(spawn.attach) {
      case  1: spawn.attach =  3; break;
      case -1: spawn.attach = -3; break;
      case  3: spawn.attach = -1; break;
      case -3: spawn.attach =  1; break;
      default: break;
    }
  }
  
  for (auto &def:feature.defs) {
    def.second.Rotate();
  }
  return feature;
}

void
LoadFeatures() {
  std::vector<std::string> assets = findAssets("features");
  for (const std::string &name : assets) {
    FILE *f = openAsset("features/"+name);
    if (f) {
      allFeatures[name] = Feature(f, name);
      allFeatures[name].variants.push_back(allFeatures[name].Rotate());
      allFeatures[name].variants.push_back(allFeatures[name].variants[0].Rotate());
      allFeatures[name].variants.push_back(allFeatures[name].variants[1].Rotate());
      allFeatures[name].variants.push_back(allFeatures[name].variants[2].Rotate());
      fclose(f);
    }
  }

  for (std::pair<std::string,Feature> f : allFeatures) {
    Log("Feature '%s'\n", f.first.c_str());
    allFeatures[f.first].ResolveConnections();
  }
}

