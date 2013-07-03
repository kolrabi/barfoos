#include "util.h"
#include "feature.h"
#include "world.h"
#include "cell.h"
#include "random.h"
#include "mob.h"
#include "game.h"
#include "vertex.h"

static std::map<std::string, Feature> allFeatures;

const Feature *getFeature(const std::string &name) {
  if (allFeatures.find(name) == allFeatures.end()) {
    std::cerr << "feature " << name << " not found" << std::endl;
    return nullptr;
  }
  return &allFeatures[name];
}

Feature::Feature(FILE *f, const std::string &name) {
  this->name = name;
  this->group = name;
  this->minLevel = 0;
  this->maxLevel = -1;
  this->maxProbability = 1.0;
  this->minLevel = 0;
  this->minY = 0;

  char line[256];
  char lastDef = 0;
  
  while(fgets(line, 256, f) && !feof(f)) {
    std::vector<std::string> tokens = Tokenize(line);
    if (tokens.size() == 0) continue;
    
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
      this->group = tokens[1];
    } else if (tokens[0] == "above") {
      this->minY = std::atoi(tokens[1].c_str());
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
      std::cerr << r.conn << " " << r.orig << " " << r.replace << std::endl;
    } else if (tokens[0] == "next") {
      conns.back().nextFeatures[tokens[1]] = 1.0f;
    } else if (tokens[0] == "nextp") {
      conns.back().nextFeatures[tokens[1]] = std::atof(tokens[2].c_str());
    } else if (tokens[0] == "def") {
      lastDef = tokens[1][0];
      defs[lastDef] = FeatureCharDef();
    } else if (tokens[0] == "cell") {
      defs[lastDef].type = tokens[1];
      if (!IsCellTypeNameValid(tokens[1])) 
        std::cerr << "unknown cell type " << tokens[1] << std::endl;
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
    } else if (tokens[0] == "mob") {
      FeatureSpawn spawn;
      spawn.spawnClass = SpawnClass::Mob;
      spawn.probability = std::atof(tokens[1].c_str());
      spawn.type = tokens[2];
      spawn.attach = std::atoi(tokens[3].c_str());
      spawn.pos = Vector3(std::atof(tokens[4].c_str()), std::atof(tokens[5].c_str()), std::atof(tokens[6].c_str()));
      this->spawns.push_back(spawn);
    } else if (tokens[0] == "entity") {
      FeatureSpawn spawn;
      spawn.spawnClass = SpawnClass::Entity;
      spawn.probability = std::atof(tokens[1].c_str());
      spawn.type = tokens[2];
      spawn.attach = std::atoi(tokens[3].c_str());
      spawn.pos = Vector3(std::atof(tokens[4].c_str()), std::atof(tokens[5].c_str()), std::atof(tokens[6].c_str()));
      this->spawns.push_back(spawn);
    } else if (tokens[0] == "slice") {
      size_t y0 = std::atof(tokens[1].c_str());
      size_t y1 = std::atof(tokens[2].c_str());
      for (size_t z=0; z<size.z; z++) {
        char *p = fgets(line, 256, f);
        if (!p) break;
        for (size_t x=0; x<size.x; x++) {
          FeatureCharDef &def = defs[line[x]];
          for (size_t y=y0; y<=y1; y++) {
            cells[x+size.x*(y+size.y*z)] = Cell(def.type);
            cells[x+size.x*(y+size.y*z)].SetYOffsets(def.top[0],def.top[1],def.top[2],def.top[3]);
            cells[x+size.x*(y+size.y*z)].SetYOffsetsBottom(def.bot[0],def.bot[1],def.bot[2],def.bot[3]);
            cells[x+size.x*(y+size.y*z)].SetOrder(def.topRev, def.botRev);
            cells[x+size.x*(y+size.y*z)].SetLocked(def.lockCell);
            cells[x+size.x*(y+size.y*z)].SetIgnoreLock(def.ignoreLock);
            cells[x+size.x*(y+size.y*z)].SetIgnoreWrite(def.ignoreWrite);
            defaultMask[x+size.x*(y+size.y*z)] = def.onlydefault;
            chars[x+size.x*(y+size.y*z)] = line[x];
          }
        }
      }
    }
    else if (tokens[0] != "") {
      std::cerr << "ignoring '" << tokens[0] << "'" << std::endl;
    }
  }

}

Feature::~Feature() {
}

const IVector3 Feature::GetSize() const {
  return size;
}

float Feature::GetProbability(const World *world, const IVector3 &pos) const {
  if (this->minY) std::cerr << pos.y << std::endl;
  if (pos.y < this->minY) {
    return 0;
  }

  int level = world->GetLevel();
  
  if (level < this->minLevel) return 0;
  if (this->maxLevel < this->minLevel) return this->maxProbability;

  // make highest chance right between min and max level
  float levelFrac = (level-minLevel)/(float)(maxLevel-minLevel);
  return maxProbability * std::sin(3.14159*levelFrac);
}

FeatureInstance Feature::BuildFeature(World *world, const IVector3 &pos, int dir, int dist, size_t id, const FeatureConnection *conn) const {
  for (size_t z=0; z<size.z; z++) {
    for (size_t y=0; y<size.y; y++) {
      for (size_t x=0; x<size.x; x++) { 
        if (defaultMask[x+size.x*(y+size.y*z)] && world->IsChecking()) continue;
        if (defaultMask[x+size.x*(y+size.y*z)] && !world->IsDefault(pos+IVector3(x,y,z))) continue;
        world->SetCell(pos+IVector3(x,y,z), cells[x+size.x*(y+size.y*z)]).SetFeatureID(id);
      }
    }
  }
  if (conn) {
    this->ReplaceChars(world, pos, conn->id, id);
  }
  return FeatureInstance(this, pos, dir, dist+1, id);
}

void Feature::ReplaceChars(World *world, const IVector3 &pos, size_t connId, size_t featureId) const {
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
          cell.SetOrder(world->GetRandom().Integer(2), world->GetRandom().Integer(2));
        } else {
          cell.SetOrder(def.topRev, def.botRev);
        }
        cell.SetLocked(def.lockCell);
        cell.SetIgnoreLock(true);
        cell.SetIgnoreWrite(def.ignoreWrite);
        world->SetCell(pos+IVector3(x,y,z), cell).SetFeatureID(featureId);
      }
    }
  }
}

void Feature::ReplaceChars(const FeatureReplacement &r, std::vector<char> &chars) const {
  for (size_t i=0; i<chars.size(); i++) {
    if (chars[i] == r.orig) chars[i] = r.replace;
  }
}

void Feature::SpawnEntities(World *world, const IVector3 &pos) const {
  for (const FeatureSpawn &spawn : spawns) {
    if (world->GetRandom().Chance(spawn.probability)) {
      Entity *entity = nullptr;

      switch(spawn.spawnClass) {
        case SpawnClass::Mob: entity = new Mob(spawn.type); break;
        case SpawnClass::Entity: entity = new Entity(spawn.type); break;
        default: continue;
      }
      
      Vector3 spawnPos = Vector3(pos)+spawn.pos+Vector3(0.5,0.5,0.5);
      if (spawn.attach) {
        IVector3 cellPos = spawnPos;
        
        if (world->GetCell(cellPos + IVector3(0,-1,0)).IsSolid() && spawn.attach == -2) {
          spawnPos.y = cellPos.y + entity->GetAABB().extents.y + 0.001;
        } else if (world->GetCell(cellPos + IVector3(1,0,0)).IsSolid() && spawn.attach == 1) {
          spawnPos.x = cellPos.x + 1-entity->GetAABB().extents.x - 0.001;
        } else if (world->GetCell(cellPos + IVector3(-1,0,0)).IsSolid() && spawn.attach == -1) {
          spawnPos.x = cellPos.x + entity->GetAABB().extents.x + 0.001;
        } else if (world->GetCell(cellPos + IVector3(0,0,1)).IsSolid() && spawn.attach == 3) {
          spawnPos.z = cellPos.z + 1-entity->GetAABB().extents.z - 0.001;
        } else if (world->GetCell(cellPos + IVector3(0,0,-1)).IsSolid() && spawn.attach == -3) {
          spawnPos.z = cellPos.z + entity->GetAABB().extents.z + 0.001;
        } else {
          continue;
        }
      }
      
      entity->SetPosition(spawnPos);
      
      Game::Instance->AddEntity(entity);
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
          if (f.second.GetGroup() == group) {
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

const Feature *FeatureConnection::GetRandomFeature(const World *world, const IVector3 &pos, Random &r) const {
  if (nextFeatures.size() == 0) return nullptr;
  
  struct W {
    const Feature *f;
    float w;
  };

  std::vector<W> totals;
  float total = 0;
  for (auto fname : nextFeatures) {
    const Feature *f = getFeature(fname.first);
    if (!f) continue;
    
    total += std::abs(f->GetProbability(world, pos+this->pos)*fname.second);
    W w;
    w.f = f;
    w.w = total;
    totals.push_back(w);
  }
  
  if (totals.size() == 0 || total == 0.0) {
    std::cerr << "argh! " << total << " " << totals.size() << std::endl;
    return nullptr;
  }

  float select = total * r.Float01();
  for (W w : totals) {
    if (select < w.w) {
      return w.f;
    }
  }
  
  std::cerr << "argh! " << std::endl;
  return nullptr;
}

const FeatureConnection *
Feature::GetRandomConnection(
  Random &r
) const {
  if (conns.size()==0) return nullptr;
  return &conns[r.Integer(conns.size())];
}

const FeatureConnection *
Feature::GetRandomConnection(
  int dir, 
  Random &r
) const {
  std::vector<const FeatureConnection *> cs;
  for (const FeatureConnection &c : conns) {
    if (c.dir == dir) cs.push_back(&c);
  }
  if (cs.size()==0) return nullptr;
  return cs[r.Integer(cs.size())];
}

void
Feature::ResolveConnections() {
  for (FeatureConnection &conn : conns) {
    conn.Resolve();
  }
}

void
LoadFeatures() {
  std::vector<std::string> assets = findAssets("features");
  for (const std::string &name : assets) {
    FILE *f = openAsset("features/"+name);
    if (f) {
      std::cerr << "loading feature " << name << std::endl;
      allFeatures[name] = Feature(f, name);
      fclose(f);
    }
  }

  for (std::pair<std::string,Feature> f : allFeatures) {
    allFeatures[f.first].ResolveConnections();
  }

}

