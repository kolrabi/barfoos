#include "util.h"
#include "feature.h"
#include "world.h"
#include "cell.h"
#include "random.h"

#include <cstring>

static std::map<std::string, Feature *> allFeatures;

const Feature *getFeature(const std::string &name) {
  if (allFeatures.find(name) == allFeatures.end()) {
    std::cerr << "feature " << name << " not found" << std::endl;
    return nullptr;
  }
  return allFeatures[name];
}

FileFeature::FileFeature(FILE *f, const std::string &name) {
  this->name = name;
  this->group = name;
  this->cells = nullptr;
  this->minLevel = 0;
  this->maxLevel = -1;
  this->maxProbability = 1.0;
  this->minLevel = 0;
  this->minY = 0;

  char line[256];

  struct Def {
    std::string type;
    float top[4]; 
    float bot[4];
    bool botRev, topRev;
    bool lockCell;
    bool ignoreLock;
    bool ignoreWrite;
    bool onlydefault;
    Def() {
      top[0] = top[1] = top[2] = top[3] = 1.0;
      bot[0] = bot[1] = bot[2] = bot[3] = 0.0;
      botRev = false;
      topRev = false;
      lockCell = true;
      ignoreLock = false;
      ignoreWrite = false;
      onlydefault = false;
    }
  };
  char lastDef = 0;
  std::map<char, Def> defs;
  
  while(fgets(line, 256, f) && !feof(f)) {
    if (line[0] == '#') continue;
    
    std::vector<std::string> tokens;
    char *p = line;
    char *q;
    do {
      q = strchr(p, ' ');
      if (!q) q = strchr(p, '\r');
      if (!q) q = strchr(p, '\n');
      if (q) *q = 0;
      tokens.push_back(p);
      if (q) { p = q+1; }
    } while(q);
    if (tokens.size() == 0) continue;

    if (tokens[0] == "size") {
      this->size = IVector3(
        std::atoi(tokens[1].c_str()), 
        std::atoi(tokens[2].c_str()), 
        std::atoi(tokens[3].c_str())
      );
      delete[]cells; 
      cells = new Cell[size.x*size.y*size.z];
      defaultMask = std::vector<bool>(size.x*size.y*size.z, false);
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
      conns.push_back(FeatureConnection(pos, dir));
    } else if (tokens[0] == "next") {
      conns.back().nextFeatures.push_back(tokens[1]);
    } else if (tokens[0] == "def") {
      lastDef = tokens[1][0];
      defs[lastDef] = Def();
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
    } else if (tokens[0] == "trev") {
      defs[lastDef].topRev = true;
    } else if (tokens[0] == "slice") {
      size_t y0 = std::atof(tokens[1].c_str());
      size_t y1 = std::atof(tokens[2].c_str());
      for (size_t z=0; z<size.z; z++) {
        char *p = fgets(line, 256, f);
        if (!p) break;
        for (size_t x=0; x<size.x; x++) {
          Def &def = defs[line[x]];
          for (size_t y=y0; y<=y1; y++) {
            cells[x+size.x*(y+size.y*z)] = Cell(def.type);
            cells[x+size.x*(y+size.y*z)].SetYOffsets(def.top[0],def.top[1],def.top[2],def.top[3]);
            cells[x+size.x*(y+size.y*z)].SetYOffsetsBottom(def.bot[0],def.bot[1],def.bot[2],def.bot[3]);
            cells[x+size.x*(y+size.y*z)].SetOrder(def.topRev, def.botRev);
            cells[x+size.x*(y+size.y*z)].SetLocked(def.lockCell);
            cells[x+size.x*(y+size.y*z)].SetIgnoreLock(def.ignoreLock);
            cells[x+size.x*(y+size.y*z)].SetIgnoreWrite(def.ignoreWrite);
            defaultMask[x+size.x*(y+size.y*z)] = def.onlydefault;
          }
        }
      }
    }
    else if (tokens[0] != "") {
      std::cerr << "ignoring '" << tokens[0] << "'" << std::endl;
    }
  }

}

FileFeature::~FileFeature() {
  delete[]cells;
}

const IVector3 FileFeature::GetSize() const {
  return size;
}

float FileFeature::GetProbability(const std::shared_ptr<World> &world, const IVector3 &pos) const {
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

FeatureInstance FileFeature::BuildFeature(const std::shared_ptr<World> &world, const IVector3 &pos, int dir, int dist) const {
  for (size_t z=0; z<size.z; z++) {
    for (size_t y=0; y<size.y; y++) {
      for (size_t x=0; x<size.x; x++) { 
        if (defaultMask[x+size.x*(y+size.y*z)] && world->IsChecking()) continue;
        if (defaultMask[x+size.x*(y+size.y*z)] && !world->IsDefault(pos+IVector3(x,y,z))) continue;
        
        world->SetCell(pos+IVector3(x,y,z), cells[x+size.x*(y+size.y*z)]);
      }
    }
  }
  return FeatureInstance(this, pos, dir, dist+1);
}

void FeatureConnection::Resolve() {
  if (!this->resolved) {
    this->resolved = true;

    auto iter = nextFeatures.begin();
    while(iter != nextFeatures.end()) {   
      if ((*iter)[0] == '$') {
        std::string group = iter->substr(1);
        iter = nextFeatures.erase(iter);
        for (auto f : allFeatures) {
          if (f.second->GetGroup() == group) {
            iter = nextFeatures.insert(iter, f.first);
          }
        }
      } else { 
        iter++;
      }
    } 
  }
} 

const Feature *FeatureConnection::GetRandomFeature(const std::shared_ptr<World> &world, const IVector3 &pos, Random &r) const {
  if (nextFeatures.size() == 0) return nullptr;
  
  struct W {
    const Feature *f;
    float w;
  };

  std::vector<W> totals;
  float total = 0;
  for (const std::string &fname : nextFeatures) {
    const Feature *f = getFeature(fname);
    if (!f) continue;
    
    total += std::abs(f->GetProbability(world, pos+this->pos));
    W w;
    w.f = f;
    w.w = total;
    totals.push_back(w);
  }
  
  if (totals.size() == 0 || total == 0.0) {
    std::cerr << "argh! " << total << std::endl;
    return getFeature(nextFeatures[r.Integer(nextFeatures.size())]);
  }

  float select = total * r.Float01();
  for (W w : totals) {
    if (select < w.w) {
      return w.f;
    }
  }
  
  return getFeature(nextFeatures[r.Integer(nextFeatures.size())]);
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
      allFeatures[name] = new FileFeature(f, name);
      fclose(f);
    }
  }

  for (auto f : allFeatures) {
    f.second->ResolveConnections();
  }

}

