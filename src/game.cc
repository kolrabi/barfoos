#include <GL/glfw.h>

#include "game.h"
#include "world.h"
#include "player.h"
#include "worldedit.h"
#include "feature.h"
#include "random.h"

extern int screenWidth;
extern int screenHeight;

Game::Game(const std::string &seed, size_t level) : seed(seed), random(seed) {
  LoadCells();
  LoadFeatures();
 
  this->BuildWorld(level);
  
  auto m = std::make_shared<Mob>(Mob());
  m->SetPosition(IVector3(32, 16, 32));
  m->SetSpawnPos(IVector3(32, 16, 32));
  this->world->AddMob(m);

  auto m2 = std::make_shared<Mob>(Mob());
  m2->SetPosition(IVector3(35, 32, 35));
  m2->SetSpawnPos(IVector3(35, 32, 35));
  this->world->AddMob(m2);
  
  this->player = std::make_shared<Player>(Player());
  this->player->SetPosition(IVector3(32,32,32));
  this->player->SetSpawnPos(IVector3(32,32,32));
  this->world->AddMob(this->player);
  this->lastT = 0;
}

Game::~Game() {
}

void
Game::Render() const {
  glClearColor(0.5,0.4,0.1, 1.0);

  // draw world first
  glViewport(0,0, screenWidth, screenHeight);
  glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(60.0f, (float)screenWidth/(float)screenHeight, 0.15f, 64.0f);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  if (glfwGetKey('Q')) {
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  } else {
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  }
  glFogi(GL_FOG_MODE, GL_EXP);
  glFogf(GL_FOG_DENSITY, 0.01f);
  player->View();
  world->Draw();
  glClear(GL_DEPTH_BUFFER_BIT);
  player->DrawWeapons();

  // next draw mini map
  if (glfwGetKey('M')) {
    glScissor(screenWidth/2-screenHeight/2,0,screenHeight,screenHeight);
    glViewport(screenWidth/2-screenHeight/2,0,screenHeight,screenHeight);
  } else {
    glViewport(screenWidth-screenWidth/8,screenHeight-screenWidth/8, screenWidth/8, screenWidth/8);
    glScissor(screenWidth-screenWidth/8,screenHeight-screenWidth/8, screenWidth/8, screenWidth/8);
  }
  glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);

  glDisable(GL_FOG);
  player->MapView();
  world->Draw();
  glEnable(GL_FOG);

  glScissor(0,0,screenWidth,screenHeight);
  glViewport(0,0,screenWidth,screenHeight);

  // next draw debug stuff
  glDisable(GL_DEPTH_TEST);
  glColor3ub(255,255,0);
  player->DrawGUI();
}

void 
Game::Update(float t) {
  if (lastT == 0) lastT = t;
  deltaT = t - lastT;
  world->Update(t);
  lastT = t;
}

void 
Game::BuildWorld(size_t level) { 
  random.Seed(seed, level); 
  std::vector<FeatureInstance> instances;
  std::map<const Feature *, int> featureCounts;

  FeatureInstance farthest;
  int loop2 = 0;
  do { 
  this->world = std::shared_ptr<World>(new World(IVector3(64, 64, 64), level, random));
  
  farthest.dist = 0;
  
  instances.clear();
  instances.push_back(getFeature("start")->BuildFeature(world, IVector3(32-4, 32-8,32-4), 0, 0));

  featureCounts.clear();
  loop2++;
  
  int loop = 0;
  do {
    if (loop++ > 10000) break;

    // select a feature from which to go
    bool useLast = random.Chance(0.90);
    size_t featNum = useLast ? instances.size()-1 : (random.Integer(instances.size()));
    const FeatureInstance &instance = instances[featNum];
    
    // select a random connection from the current feature
    const Feature *feature = instance.feature;
    const FeatureConnection *conn = feature->GetRandomConnection(random);
    if (!conn) continue;

    // select next feature
    const Feature *nextFeature = conn->GetRandomFeature(world, instance.pos, random);
    if (!nextFeature) continue;
    
    // make sure both features can connect
    const FeatureConnection *revConn = nextFeature->GetRandomConnection(-conn->dir, random);
    if (!revConn) continue;

    // snap both connection points together
    IVector3 pos = instance.pos + conn->pos - revConn->pos;
    
    // build the next feature if possible
    this->world->BeginCheckOverwrite();
    nextFeature->BuildFeature(this->world, pos, conn->dir, instance.dist);
    if (this->world->FinishCheckOverwrite()) {
      instances.push_back(nextFeature->BuildFeature(world, pos, conn->dir, instance.dist));
      if (instances.back().dist > farthest.dist) farthest = instances.back();
      if (featureCounts.find(nextFeature) == featureCounts.end()) {
        featureCounts[nextFeature] = 1;
      } else {
        featureCounts[nextFeature] ++;
      }
    }
  } while(instances.size() < 500 && loop2<10); 
  if (instances.size() < 50)
  std::cerr << instances.size() << " features is not enough" << std::endl;
  //seed += 1024;
  } while(instances.size() < 100);

  std::cerr << "built world with " << instances.size() << " features. seed " << (seed) << std::endl;
  for (auto fc : featureCounts) {
    std::cerr << "  " << fc.first->GetName() << ": " << fc.second << std::endl;
  }
 
  WorldEdit e(this->world);
  
  // create caves
  size_t caveCount = random.Integer(30)+5;
  
  e.SetBrush(Cell("air"));

  for (size_t i = 0; i<caveCount; i++) {
    size_t featNum = random.Integer(instances.size());
    const FeatureInstance &instance = instances[featNum];
    IVector3 size = instance.feature->GetSize();
   
    IVector3 cavePos(instance.pos + IVector3(random.Integer(size.x), random.Integer(size.y), random.Integer(size.z)));
    size_t caveLength = random.Integer(3000);
    bool lastSolid = false;
    
    for (size_t j=0; j<caveLength; j++) {
      Side nextSide = (Side)(random.Integer(6));
      bool solid = this->world->GetCell(cavePos).GetInfo().flags & CellFlags::Solid;
      if (solid && !lastSolid) {
        e.ApplyBrush(cavePos);
      }
      lastSolid = solid;
      cavePos = cavePos[nextSide];
    }
  }
  
  IVector3 endPos(farthest.pos + farthest.feature->GetSize()*0.5);
  this->world->SetCell(endPos, Cell("torch3"));

  world->Dump();
}

void Game::MouseClick(int button, bool down) {
  this->player->MouseClick(button, down);
}
